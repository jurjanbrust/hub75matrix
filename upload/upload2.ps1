# PowerShell script to test /api/gif/upload endpoint

$endpoint = "http://192.168.10.22/api/gif/upload"
$filePath = "C:\Dev\IoT\Hub75_matrix\myhub75matrix\upload\windows.gif"
$fileName = "windows.gif"

# Create multipart form data content
$boundary = "----WebKitFormBoundary" + [System.Guid]::NewGuid().ToString("N")
$LF = "`r`n"
$bodyLines = @(
    "--$boundary",
    "Content-Disposition: form-data; name=`"file`"; filename=`"$fileName`"",
    "Content-Type: image/gif$LF",
    [System.IO.File]::ReadAllText($filePath),
    "--$boundary--$LF"
)
$body = [System.Text.Encoding]::UTF8.GetBytes(($bodyLines -join $LF))

Invoke-WebRequest -Uri $endpoint -Method Post -ContentType "multipart/form-data; boundary=$boundary" -Body $body