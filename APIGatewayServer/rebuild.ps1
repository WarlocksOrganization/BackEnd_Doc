# rebuild.ps1
# Script to delete the build directory, recreate it, regenerate the CMake cache, and perform a build

# Check and save current location
$currentDir = Get-Location

# Verify we are in the project root directory
if (-not (Test-Path ".\CMakeLists.txt")) {
    Write-Host "Error: This script must be run from the project root directory." -ForegroundColor Red
    Write-Host "Please run the script from the directory containing CMakeLists.txt." -ForegroundColor Red
    exit 1
}

Write-Host "===== Starting Clean Build and Reconfiguration Script =====" -ForegroundColor Cyan

# 1. Delete build directory
Write-Host "1. Deleting build directory..." -ForegroundColor Yellow
if (Test-Path ".\build") {
    Remove-Item -Recurse -Force .\build
    Write-Host "   Existing build directory deleted" -ForegroundColor Green
} else {
    Write-Host "   Build directory does not exist. Creating a new one." -ForegroundColor Yellow
}

# 2. Create new build directory
Write-Host "2. Creating new build directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path .\build | Out-Null
Write-Host "   Build directory created" -ForegroundColor Green

# 3. Navigate to build directory
Set-Location -Path .\build

# 4. Generate CMake cache
Write-Host "3. Generating CMake cache..." -ForegroundColor Yellow
cmake ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "   CMake cache generation failed" -ForegroundColor Red
    Set-Location -Path $currentDir  # Return to original location
    exit 1
}
Write-Host "   CMake cache generation completed" -ForegroundColor Green

# 5. Perform build
Write-Host "4. Performing build..." -ForegroundColor Yellow
cmake --build . --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "   Build failed" -ForegroundColor Red
} else {
    Write-Host "   Build successful" -ForegroundColor Green
}

# 6. Return to original location
Set-Location -Path $currentDir

# 7. Run Server
.\build\Release\SocketServer.exe

Write-Host "===== Clean Build and Reconfiguration Script Completed =====" -ForegroundColor Cyan