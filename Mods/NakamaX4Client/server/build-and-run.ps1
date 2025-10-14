# PowerShell script to build and run Nakama server with Go modules

Write-Host "Building Nakama Go server module..." -ForegroundColor Cyan

# Navigate to server directory
$serverDir = $PSScriptRoot
Set-Location $serverDir

# Check if Docker is running
$dockerRunning = docker info 2>$null
if (-not $dockerRunning) {
    Write-Host "ERROR: Docker is not running. Please start Docker Desktop." -ForegroundColor Red
    exit 1
}

# Stop existing containers
Write-Host "Stopping existing containers..." -ForegroundColor Yellow
docker-compose down

# Build the Go plugin
Write-Host "Building Go plugin..." -ForegroundColor Yellow
docker-compose build

# Start the services
Write-Host "Starting Nakama server..." -ForegroundColor Green
docker-compose up -d

# Wait for Nakama to be ready
Write-Host "Waiting for Nakama to initialize..." -ForegroundColor Cyan
Start-Sleep -Seconds 5

# Check if services are running
$nakamaRunning = docker ps --filter "name=nakama" --filter "status=running" --format "{{.Names}}"
if ($nakamaRunning -eq "nakama") {
    Write-Host "`n✅ Nakama server is running!" -ForegroundColor Green
    Write-Host "`nServer endpoints:" -ForegroundColor Cyan
    Write-Host "  - HTTP API:     http://localhost:7350" -ForegroundColor White
    Write-Host "  - gRPC API:     localhost:7349" -ForegroundColor White
    Write-Host "  - Console:      http://localhost:7351 (admin/password)" -ForegroundColor White
    Write-Host "`nTo view logs: docker-compose logs -f nakama" -ForegroundColor Yellow
    Write-Host "To stop:      docker-compose down`n" -ForegroundColor Yellow
} else {
    Write-Host "`n❌ Failed to start Nakama server" -ForegroundColor Red
    Write-Host "Check logs with: docker-compose logs nakama`n" -ForegroundColor Yellow
    exit 1
}
