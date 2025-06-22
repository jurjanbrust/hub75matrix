<#
.SYNOPSIS
    Uploads all files from the data folder and its subfolders to the ESP32 device.

.DESCRIPTION
    This script recursively finds all files in the data folder and uploads them to the ESP32 device
    using the appropriate API endpoints. It handles different file types and creates the necessary
    directory structure on the device.

.PARAMETER DataFolderPath
    The path to the data folder containing files to upload.
    Default: ..\data (relative to the script location)

.PARAMETER TargetUrl
    The base URL of the ESP32 device.
    Default: http://192.168.10.22

.PARAMETER SkipExisting
    Skip files that already exist on the device (based on file size comparison).
    Default: $false

.EXAMPLE
    .\upload.ps1

.EXAMPLE
    .\upload.ps1 -DataFolderPath "C:\MyProject\data" -TargetUrl "http://192.168.1.100"

.EXAMPLE
    .\upload.ps1 -SkipExisting $true

.NOTES
    This script requires the ESP32 device to be running and accessible via HTTP.
    It will create directories as needed and upload all files found in the data folder.
#>
param
(
    [string]$DataFolderPath = "..\data",
    [string]$TargetUrl = "http://matrix.local",
    [bool]$SkipExisting = $false
)

# Resolve the full path to the data folder
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$fullDataPath = Resolve-Path (Join-Path $scriptDir $DataFolderPath) -ErrorAction SilentlyContinue

if (-not $fullDataPath) {
    Write-Error "Error: Data folder not found at '$DataFolderPath'"
    Write-Host "Current script directory: $scriptDir"
    Write-Host "Attempted path: $(Join-Path $scriptDir $DataFolderPath)"
    exit 1
}

Write-Host "Uploading all files from: $fullDataPath"
Write-Host "Target device: $TargetUrl"
Write-Host "Skip existing files: $SkipExisting"
Write-Host ""

# Function to get file size from device
function Get-DeviceFileSize {
    param([string]$FilePath)
    
    try {
        $response = Invoke-RestMethod -Uri "$TargetUrl/api/files" -Method Get -TimeoutSec 10
        $fileName = Split-Path $FilePath -Leaf
        $file = $response | Where-Object { $_.name -eq $fileName }
        if ($file) {
            return $file.size
        }
    }
    catch {
        # If API call fails, assume file doesn't exist
        return $null
    }
    return $null
}

# Function to create directory on device
function New-DeviceDirectory {
    param([string]$DirectoryPath)
    
    try {
        $body = @{
            name = $DirectoryPath.TrimStart('/')
        } | ConvertTo-Json
        
        $response = Invoke-RestMethod -Uri "$TargetUrl/api/files/create-folder" -Method Post -Body $body -ContentType "application/json" -TimeoutSec 10
        Write-Host "Created directory: $DirectoryPath" -ForegroundColor Green
        return $true
    }
    catch {
        if ($_.Exception.Response.StatusCode -eq 409) {
            # Directory already exists
            return $true
        }
        Write-Warning "Failed to create directory '$DirectoryPath': $($_.Exception.Message)"
        return $false
    }
}

# Function to upload a single file
function Upload-File {
    param([string]$FilePath, [string]$TargetPath)
    
    $fileName = Split-Path $FilePath -Leaf
    $fileSize = (Get-Item $FilePath).Length
    
    #if filename is environment.js replace it with environment-esp32.js
    if ($fileName -eq "environment.js") {
        Write-Host "Skipping: $fileName ($fileSize bytes) to $TargetPath" -NoNewline
        return $true
    }

    Write-Host "Uploading: $fileName ($fileSize bytes) to $TargetPath" -NoNewline
    
    # Check if file already exists and skip if requested
    if ($SkipExisting) {
        $deviceFileSize = Get-DeviceFileSize -FilePath $TargetPath
        if ($deviceFileSize -eq $fileSize) {
            Write-Host " - SKIPPED (same size)" -ForegroundColor Yellow
            return $true
        }
    }
    
    try {
        # Create form data for file upload
        $form = @{
            file = Get-Item -Path $FilePath
            path = $TargetPath
        }
        
        # Upload the file using the general file upload endpoint
        $response = Invoke-RestMethod -Uri "$TargetUrl/api/file/upload" -Method Post -Form $form -TimeoutSec 60
        
        if ($response.status -eq "success") {
            Write-Host " - SUCCESS" -ForegroundColor Green
            return $true
        } else {
            Write-Host " - FAILED: $($response.message)" -ForegroundColor Red
            return $false
        }
    }
    catch {
        Write-Host " - ERROR: $($_.Exception.Message)" -ForegroundColor Red
        return $false
    }
}

# Function to process directory recursively
function Process-Directory {
    param([string]$LocalPath, [string]$DevicePath)
    
    Write-Host "Processing directory: $LocalPath -> $DevicePath" -ForegroundColor Cyan
    
    # Get all items in the directory
    $items = Get-ChildItem -Path $LocalPath -Force
    
    foreach ($item in $items) {
        $relativePath = $item.FullName.Substring($fullDataPath.Length).Replace('\', '/')
        $deviceItemPath = $DevicePath + $relativePath
        
        if ($item.PSIsContainer) {
            # It's a directory
            if (New-DeviceDirectory -DirectoryPath $deviceItemPath) {
                Process-Directory -LocalPath $item.FullName -DevicePath $DevicePath
            }
        } else {
            # It's a file
            if (Upload-File -FilePath $item.FullName -TargetPath $deviceItemPath) {
                $script:successCount++
            } else {
                $script:errorCount++
            }
        }
    }
}

# Main execution
try {
    # Test connection to device
    Write-Host "Testing connection to device..." -NoNewline
    $status = Invoke-RestMethod -Uri "$TargetUrl/api/status" -Method Get -TimeoutSec 10
    Write-Host " SUCCESS" -ForegroundColor Green
    Write-Host "Device status: $($status.status)"
    Write-Host "Device IP: $($status.ip)"
    Write-Host ""
    
    # Start processing
    $startTime = Get-Date
    $script:successCount = 0
    $script:errorCount = 0
    
    # Process the data directory
    Process-Directory -LocalPath $fullDataPath -DevicePath "/data"
    
    $endTime = Get-Date
    $duration = $endTime - $startTime
    
    Write-Host ""
    Write-Host "Upload completed in $($duration.TotalSeconds.ToString('F1')) seconds"
    Write-Host "Files uploaded successfully: $script:successCount"
    if ($script:errorCount -gt 0) {
        Write-Host "Files with errors: $script:errorCount" -ForegroundColor Yellow
    }
    
}
catch {
    Write-Error "Failed to connect to device at $TargetUrl"
    Write-Error "Error: $($_.Exception.Message)"
    Write-Host ""
    Write-Host "Please ensure:"
    Write-Host "1. The ESP32 device is powered on and connected to WiFi"
    Write-Host "2. The device IP address is correct"
    Write-Host "3. The device is accessible from this computer"
    exit 1
}
