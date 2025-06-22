# PowerShell script to copy web interface files to SD card
# Make sure to run this script after mounting the SD card

$sourceDir = ".\data"
$targetDir = "D:\data"  # Change this to match your SD card drive letter

# Create target directory if it doesn't exist
if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Path $targetDir -Force
}

# Copy all files and subdirectories
Copy-Item -Path "$sourceDir\*" -Destination $targetDir -Recurse -Force

# copy environment-esp32 to esp32
Copy-Item -Path "$sourceDir\js\environment-esp32.js" -Destination "$targetDir\js\environment.js" -Force
Remove-Item "$targetDir\js\environment-esp32.js"

Write-Host "Web interface files copied to SD card successfully!" 