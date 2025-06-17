# PowerShell script to test /api/gif/upload endpoint

$endpoint = "http://192.168.10.22/api/gif/upload" # Replace <DEVICE_IP> with your device's IP

# 1. Successful GIF upload
Write-Host "Testing valid GIF upload..."
Invoke-RestMethod -Uri $endpoint -Method Post -Form @{
    "filename" = "test.gif"
    "file" = Get-Item "C:\Path\To\Your\test.gif"
} | ConvertTo-Json

# 2. Upload with wrong extension
Write-Host "Testing upload with wrong extension..."
Invoke-RestMethod -Uri $endpoint -Method Post -Form @{
    "filename" = "test.txt"
    "file" = Get-Item "C:\Path\To\Your\test.gif"
} | ConvertTo-Json

# 3. Upload with missing filename
Write-Host "Testing upload with missing filename..."
Invoke-RestMethod -Uri $endpoint -Method Post -Form @{
    "file" = Get-Item "C:\Path\To\Your\test.gif"
} | ConvertTo-Json