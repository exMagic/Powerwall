# PWServer - .NET Web API with Docker Support

A .NET 8 Web API project with Docker containerization support.

## Prerequisites

- .NET 8 SDK
- Docker Desktop
- Docker Compose

## Project Structure

```
PWServer/
├── PWServer/                 # Main project directory
│   ├── Controllers/          # API Controllers
│   ├── Properties/           # Project properties
│   ├── appsettings.json      # Application settings
│   ├── Program.cs           # Application entry point
│   ├── PWServer.csproj      # Project file
│   ├── Dockerfile           # Docker configuration
│   └── .dockerignore        # Docker ignore file
├── docker-compose.yml        # Production Docker Compose
├── docker-compose.dev.yml    # Development Docker Compose
└── README.md                # This file
```

## Running the Application

### Local Development (without Docker)

1. Navigate to the PWServer directory:
   ```powershell
   cd PWServer
   ```

2. Restore dependencies:
   ```powershell
   dotnet restore
   ```

3. Run the application:
   ```powershell
   dotnet run
   ```

The API will be available at:
- HTTP: http://localhost:5000
- HTTPS: https://localhost:5001

### Docker Development

1. Run with Docker Compose (development mode with hot reload):
   ```powershell
   docker-compose -f docker-compose.dev.yml up --build
   ```

2. The API will be available at:
   - HTTP: http://localhost:8080
   - HTTPS: https://localhost:8081

### Docker Production

1. Run with Docker Compose (production mode):
   ```powershell
   docker-compose up --build
   ```

2. The API will be available at:
   - HTTP: http://localhost:8080
   - HTTPS: https://localhost:8081

## API Endpoints

The default template includes:

- `GET /weatherforecast` - Returns sample weather data
- Swagger UI available at `/swagger` (in development mode)

## Docker Commands

### Build the Docker image manually:
```powershell
docker build -t pwserver ./PWServer
```

### Run the container manually:
```powershell
docker run -p 8080:8080 -p 8081:8081 pwserver
```

### Stop all services:
```powershell
docker-compose down
```

### View logs:
```powershell
docker-compose logs -f pwserver
```

## Development

### Adding New Controllers

1. Create a new controller in the `Controllers` directory
2. Inherit from `ControllerBase` and add the `[ApiController]` attribute
3. Add your API endpoints with appropriate HTTP method attributes

### Configuration

- Modify `appsettings.json` for application settings
- Environment-specific settings can be added in `appsettings.Development.json`
- Docker environment variables can be configured in the docker-compose files

## Troubleshooting

### HTTPS Certificate Issues

If you encounter HTTPS certificate issues in Docker, you may need to generate development certificates:

```powershell
dotnet dev-certs https -ep %USERPROFILE%\.aspnet\https\aspnetapp.pfx -p password
dotnet dev-certs https --trust
```

### Port Conflicts

If ports 8080 or 8081 are already in use, modify the port mappings in the docker-compose files.

## Next Steps

1. Add your business logic and controllers
2. Configure a database (Entity Framework Core recommended)
3. Add authentication and authorization
4. Implement logging and monitoring
5. Add unit and integration tests