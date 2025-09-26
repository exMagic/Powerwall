# Powerwall Dev Commands (Docker + API + TimescaleDB)

A handy cheat sheet for running the dev stack, testing the API, and inspecting the TimescaleDB database.

## Prerequisites
- Docker Desktop running
- PowerShell (these examples are tailored for Windows PowerShell)

Project root for commands below:

```powershell
cd C:\Repos\Powerwall\Powerwall\PWServer
```

---

## Docker (Dev Stack)

Start (foreground, shows logs):
```powershell
docker-compose -f docker-compose.dev.yml up --build
```

Start (detached):
```powershell
docker-compose -f docker-compose.dev.yml up --build -d
```

Stop and remove containers:
```powershell
docker-compose -f docker-compose.dev.yml down
```

See status:
```powershell
docker-compose -f docker-compose.dev.yml ps
```

Follow logs (API):
```powershell
docker-compose -f docker-compose.dev.yml logs -f pwserver
```

Follow logs (DB):
```powershell
docker-compose -f docker-compose.dev.yml logs -f timescaledb
```

Rebuild only the API service:
```powershell
docker-compose -f docker-compose.dev.yml build pwserver
```

List running containers and ports:
```powershell
docker ps --format "table {{.Names}}\t{{.Ports}}\t{{.Status}}"
```

Test that API port is reachable:
```powershell
Test-NetConnection -ComputerName localhost -Port 8080
```

---

## API Endpoints (PowerShell)
Base URL in dev: http://localhost:8080

Health check (DB):
```powershell
Invoke-RestMethod http://localhost:8080/api/health/db
```

Create a new sensor id and ingest a measurement:
```powershell
$sensorId = [guid]::NewGuid().ToString()
$body = @{ Voltage = 48.5; Current = 12.3; TemperatureC = 24.7; EnergyWh = 150.0 } | ConvertTo-Json
Invoke-RestMethod -Method POST -Uri "http://localhost:8080/api/sensors/$sensorId/measurements" -ContentType "application/json" -Body $body
```

Read raw measurements in a time range (last 10 minutes):
```powershell
$from = (Get-Date).AddMinutes(-10).ToUniversalTime().ToString("o")
$to   = (Get-Date).AddMinutes(10).ToUniversalTime().ToString("o")
Invoke-RestMethod "http://localhost:8080/api/sensors/$sensorId/measurements?from=$from&to=$to"
```

Read aggregated measurements (5-minute buckets):
```powershell
$bucket = [uri]::EscapeDataString('5 minutes')  # encodes space as %20
Invoke-RestMethod "http://localhost:8080/api/sensors/$sensorId/measurements?from=$from&to=$to&bucket=$bucket"
```

Swagger UI (browse endpoints):
```powershell
Start-Process http://localhost:8080/swagger
```

---

## TimescaleDB (psql inside container)
Container name: `timescaledb`
Database: `powerwall`, User: `postgres`, Password: `postgres`

Open interactive psql:
```powershell
docker exec -it timescaledb psql -U postgres -d powerwall
```

Run one-off SQL commands:
```powershell
docker exec -it timescaledb psql -U postgres -d powerwall -c "SELECT 1;"
```

Useful psql meta-commands (run inside psql):
```sql
-- list tables
\dt

-- list extensions
\dx

-- describe table
\d+ measurements
```

Check Timescale extension (and create if missing):
```powershell
docker exec -it timescaledb psql -U postgres -d powerwall -c "CREATE EXTENSION IF NOT EXISTS timescaledb;"
```

Confirm hypertable exists:
```powershell
docker exec -it timescaledb psql -U postgres -d powerwall -c "SELECT hypertable_name FROM timescaledb_information.hypertables;"
```

Peek at recent data:
```powershell
docker exec -it timescaledb psql -U postgres -d powerwall -c "SELECT time, sensor_id, voltage, current, temperature_c, energy_wh FROM measurements ORDER BY time DESC LIMIT 10;"
```

Example aggregate query (5-minute buckets, last 24h):
```powershell
docker exec -it timescaledb psql -U postgres -d powerwall -c "\
  SELECT time_bucket('5 minutes', time) AS bucket,\
         AVG(voltage) AS avg_voltage,\
         AVG(current) AS avg_current,\
         AVG(temperature_c) AS avg_temperature_c,\
         MAX(energy_wh) - MIN(energy_wh) AS energy_delta_wh\
  FROM measurements\
  WHERE time >= now() - interval '24 hours'\
  GROUP BY bucket\
  ORDER BY bucket;" 
```

---

## Local runs (without Docker)
From the project folder:
```powershell
cd .\PWServer
dotnet restore
dotnet run
```

API defaults:
- http://localhost:5000
- https://localhost:5001 (if dev certs installed)

Optional: create and trust dev HTTPS certs:
```powershell
dotnet dev-certs https -ep $env:USERPROFILE\.aspnet\https\aspnetapp.pfx -p password
dotnet dev-certs https --trust
```

---

## Maintenance & Troubleshooting

Restart only the API container:
```powershell
docker-compose -f docker-compose.dev.yml restart pwserver
```

Tail logs and retry a request:
```powershell
docker-compose -f docker-compose.dev.yml logs -f pwserver
```

Check that port 8080 is bound:
```powershell
docker ps --format "table {{.Names}}\t{{.Ports}}\t{{.Status}}"
Test-NetConnection -ComputerName localhost -Port 8080
```

Reset the database (DANGER: deletes all data):
```powershell
docker-compose -f docker-compose.dev.yml down
docker volume rm timescale-data
```

If aggregation errors but raw works, ensure Timescale extension exists:
```powershell
docker exec -it timescaledb psql -U postgres -d powerwall -c "CREATE EXTENSION IF NOT EXISTS timescaledb;"
```

If the API returns 500, capture logs and share:
```powershell
docker-compose -f docker-compose.dev.yml logs --tail=200 pwserver
```

---

## Schema Summary
- `sensors(id uuid primary key, name text, created_at timestamptz)`
- `measurements(time timestamptz, sensor_id uuid fk, voltage double precision, current double precision, temperature_c double precision, energy_wh double precision, primary key(sensor_id, time))`
- `measurements` is a Timescale hypertable on `time`

You can modify retention/compression and continuous aggregates later for performance.
