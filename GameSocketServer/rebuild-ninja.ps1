# fast-build.ps1
# Script for optimized C++ build with architecture correction

# Parameter to support clean build option
param (
    [switch]$Clean = $false
)

# Check and save current location
$currentDir = Get-Location

# Verify we are in the project root directory
if (-not (Test-Path ".\CMakeLists.txt")) {
    Write-Host "Error: This script must be run from the project root directory." -ForegroundColor Red
    Write-Host "Please run the script from the directory containing CMakeLists.txt." -ForegroundColor Red
    exit 1
}

Write-Host "===== Starting Optimized Build Script =====" -ForegroundColor Cyan

# Check if Ninja is installed
$ninjaExists = $null -ne (Get-Command ninja -ErrorAction SilentlyContinue)

# Determine if we need a clean build
if ($Clean) {
    Write-Host "Clean build requested." -ForegroundColor Yellow
    if (Test-Path ".\build") {
        Write-Host "Removing existing build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force .\build
        Write-Host "Build directory removed" -ForegroundColor Green
    }
}

# Create build directory if needed
if (-not (Test-Path ".\build")) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path .\build | Out-Null
    Write-Host "Build directory created" -ForegroundColor Green
}

# Navigate to build directory
Set-Location -Path .\build

# Detect CPU cores for parallel build
$cpuCores = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
$buildParallelism = [Math]::Max(1, $cpuCores - 1)  # Use N-1 cores to keep system responsive

# Configure CMake based on available build system and architecture
Write-Host "Configuring CMake with optimizations..." -ForegroundColor Yellow

# For simplicity and reliability, use Visual Studio generator
$cmakeCmd = "cmake -G `"Visual Studio 17 2022`" -A x64 .."
Write-Host "Using Visual Studio generator with x64 architecture" -ForegroundColor Yellow
Write-Host "Running: $cmakeCmd" -ForegroundColor Gray
Invoke-Expression $cmakeCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed" -ForegroundColor Red
    Set-Location -Path $currentDir
    exit 1
}
Write-Host "CMake configuration succeeded" -ForegroundColor Green

# Perform the build
Write-Host "Building with $buildParallelism parallel jobs..." -ForegroundColor Yellow

$buildCmd = "cmake --build . --config Release --parallel $buildParallelism"
Write-Host "Running: $buildCmd" -ForegroundColor Gray
Invoke-Expression $buildCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed" -ForegroundColor Red
} else {
    Write-Host "Build successful" -ForegroundColor Green
    
    # Copy executable to root directory for easier access
    if (Test-Path ".\Release\SocketServer.exe") {
        Copy-Item ".\Release\SocketServer.exe" "..\SocketServer.exe" -Force
        Write-Host "Executable copied to root directory" -ForegroundColor Green
    }
}

# Return to original location
Set-Location -Path $currentDir

Write-Host "===== Optimized Build Completed =====" -ForegroundColor Cyan

# Display execution suggestions
Write-Host "To run the server: .\SocketServer.exe" -ForegroundColor Cyan
Write-Host "For a clean rebuild: .\fast-build.ps1 -Clean" -ForegroundColor Cyan