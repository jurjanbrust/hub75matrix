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

Write-Host "Web interface files copied to SD card successfully!" 