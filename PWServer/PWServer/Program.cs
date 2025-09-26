using System.Data;
using Dapper;
using Npgsql;

var builder = WebApplication.CreateBuilder(args);

// Dapper: allow mapping snake_case columns to PascalCase properties
Dapper.DefaultTypeMap.MatchNamesWithUnderscores = true;

// Add services to the container.
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

// Register a scoped Npgsql connection per request (do not open here)
builder.Services.AddScoped<IDbConnection>(_ =>
{
    var cs = builder.Configuration.GetConnectionString("Timescale")
             ?? builder.Configuration["ConnectionStrings:Timescale"]
             ?? "Host=localhost;Port=5432;Database=powerwall;Username=postgres;Password=postgres;Pooling=true;Timezone=UTC";
    return new NpgsqlConnection(cs);
});

var app = builder.Build();

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.UseDeveloperExceptionPage();
    app.UseSwagger();
    app.UseSwaggerUI();
}

// No HTTPS redirection in container dev by default

// Ensure DB schema exists and convert to hypertable at startup.
await EnsureDatabaseAsync(app.Services);

// Simple sensor ingestion and readback endpoints
app.MapPost("/api/sensors/{sensorId:guid}/measurements", async (Guid sensorId, MeasurementIn body, IDbConnection db, ILogger<Program> logger, CancellationToken ct) =>
{
    try
    {
    if (db is NpgsqlConnection npg) await npg.OpenAsync(ct);
        // Ensure sensor exists (create on first write with default name)
        const string ensureSensorSql = @"INSERT INTO sensors (id, name) VALUES (@sensorId, @name)
                                        ON CONFLICT (id) DO NOTHING";
        await db.ExecuteAsync(new CommandDefinition(ensureSensorSql, new { sensorId, name = $"sensor-{sensorId}" }, cancellationToken: ct));

        // Insert a measurement row
        const string sql = @"INSERT INTO measurements (time, sensor_id, voltage, current, temperature_c, energy_wh)
                             VALUES (@time, @sensorId, @voltage, @current, @temperature_c, @energy_wh)";

        var now = body.TimeUtc ?? DateTime.UtcNow;

        var parameters = new
        {
            time = now,
            sensorId,
            voltage = body.Voltage,
            current = body.Current,
            temperature_c = body.TemperatureC,
            energy_wh = body.EnergyWh
        };

    await db.ExecuteAsync(new CommandDefinition(sql, parameters, cancellationToken: ct));
        return Results.Accepted($"/api/sensors/{sensorId}/measurements?from={now:o}");
    }
    catch (Exception ex)
    {
        logger.LogError(ex, "Failed to ingest measurement for {SensorId}", sensorId);
        return Results.Problem(ex.Message);
    }
})
.WithName("IngestMeasurement")
.WithOpenApi();

app.MapGet("/api/sensors/{sensorId:guid}/measurements", async (Guid sensorId, DateTime? from, DateTime? to, string? bucket, IDbConnection db, ILogger<Program> logger, CancellationToken ct) =>
{
    try
    {
    if (db is NpgsqlConnection npg) await npg.OpenAsync(ct);
        // If bucket is provided (e.g., '5 minutes', '1 hour'), return aggregated; otherwise return raw.
        from ??= DateTime.UtcNow.AddHours(-1);
        to ??= DateTime.UtcNow;

        if (!string.IsNullOrWhiteSpace(bucket))
        {
            const string aggSql = @"SELECT
                  time_bucket(CAST(@bucket AS interval), time) AS bucket,
                  AVG(voltage)      AS avg_voltage,
                  AVG(current)      AS avg_current,
                  AVG(temperature_c) AS avg_temperature_c,
                  MAX(energy_wh) - MIN(energy_wh) AS energy_delta_wh
                FROM measurements
                WHERE sensor_id = @sensorId AND time BETWEEN @from AND @to
                GROUP BY bucket
                ORDER BY bucket";

            var rows = await db.QueryAsync(new CommandDefinition(aggSql, new { bucket, sensorId, from, to }, cancellationToken: ct));
            return Results.Ok(rows);
        }
        else
        {
            const string rawSql = @"SELECT time, voltage, current, temperature_c, energy_wh
                                    FROM measurements
                                    WHERE sensor_id = @sensorId AND time BETWEEN @from AND @to
                                    ORDER BY time";
            var rows = await db.QueryAsync<MeasurementOut>(new CommandDefinition(rawSql, new { sensorId, from, to }, cancellationToken: ct));
            return Results.Ok(rows);
        }
    }
    catch (Exception ex)
    {
        logger.LogError(ex, "Failed to get measurements for {SensorId}", sensorId);
        return Results.Problem(ex.Message);
    }
})
.WithName("GetMeasurements")
.WithOpenApi();

// DB health check endpoint
app.MapGet("/api/health/db", async (IDbConnection db) =>
{
    if (db is NpgsqlConnection npg) await npg.OpenAsync();
    var val = await db.QuerySingleAsync<int>("select 1");
    return Results.Ok(new { ok = val == 1 });
})
.WithName("DbHealth")
.WithOpenApi();

app.Run();


static async Task EnsureDatabaseAsync(IServiceProvider services)
{
    using var scope = services.CreateScope();
        var cs = scope.ServiceProvider.GetRequiredService<IConfiguration>().GetConnectionString("Timescale")
                             ?? scope.ServiceProvider.GetRequiredService<IConfiguration>()["ConnectionStrings:Timescale"]
                         ?? "Host=localhost;Port=5432;Database=powerwall;Username=postgres;Password=postgres;Pooling=true;Timezone=UTC";
        await using var db = new NpgsqlConnection(cs);
        await db.OpenAsync();

        // Enable required extensions and create schema
        var sql = @"
        DO $$ BEGIN
            CREATE EXTENSION IF NOT EXISTS timescaledb;
        EXCEPTION WHEN OTHERS THEN
            -- in case of permissions issues, ignore
            RAISE NOTICE 'timescaledb extension creation skipped: %', SQLERRM;
        END $$;

    CREATE TABLE IF NOT EXISTS sensors (
        id uuid PRIMARY KEY,
        name text NOT NULL,
        created_at timestamptz NOT NULL DEFAULT now()
    );

    CREATE TABLE IF NOT EXISTS measurements (
        time timestamptz NOT NULL,
        sensor_id uuid NOT NULL REFERENCES sensors(id) ON DELETE CASCADE,
        voltage double precision NULL,
        current double precision NULL,
        temperature_c double precision NULL,
        energy_wh double precision NULL,
        PRIMARY KEY (sensor_id, time)
    );

    SELECT create_hypertable('measurements', 'time', if_not_exists => true);

    -- Optional index to speed up sensor queries by time
    CREATE INDEX IF NOT EXISTS idx_measurements_sensor_time ON measurements (sensor_id, time DESC);
    ";

    await db.ExecuteAsync(sql);
}

// DTOs
public record MeasurementIn(
    DateTime? TimeUtc,
    double? Voltage,
    double? Current,
    double? TemperatureC,
    double? EnergyWh
);

public record MeasurementOut(
    DateTime Time,
    double? Voltage,
    double? Current,
    double? TemperatureC,
    double? EnergyWh
);
