<#
.SYNOPSIS
    Uploads a GIF file to a specified endpoint.

.DESCRIPTION
    This script takes the path to a GIF file and uploads it to a given HTTP endpoint
    using a POST request. It is designed to be flexible, allowing easy modification
    of the target URL and file path.

.PARAMETER GifFilePath
    The full path to the GIF file you want to upload.
    Example: C:\Users\YourUser\Pictures\my_awesome_gif.gif

.PARAMETER TargetUrl
    The URL of the endpoint where the GIF will be uploaded.
    Default: http://192.168.10.22/api/gif/upload

.EXAMPLE
    .\Upload-Gif.ps1 -GifFilePath "C:\gifs\funny.gif"

.EXAMPLE
    .\Upload-Gif.ps1 -GifFilePath "C:\gifs\animation.gif" -TargetUrl "http://another.server.com/upload"

.NOTES
    Ensure the target endpoint is expecting a file upload in the body of a POST request.
    This script assumes the endpoint can handle raw file content or a multipart/form-data
    if the body is modified to include form data. For simplicity, this version sends
    the raw file content. If the server expects 'multipart/form-data',
    additional parameters will be needed for Invoke-RestMethod.
#>
param
(
    [Parameter(Mandatory=$true)]
    [string]$GifFilePath,

    [string]$TargetUrl = "http://192.168.10.22/api/gif/upload"
)

# Check if the GIF file exists
if (-not (Test-Path -Path $GifFilePath -PathType Leaf)) {
    Write-Error "Error: The specified GIF file does not exist at '$GifFilePath'."
    exit 1
}

Write-Host "Attempting to upload '$GifFilePath' to '$TargetUrl'..."

try {
    # Read the GIF file content as bytes
    $fileContent = [System.IO.File]::ReadAllBytes($GifFilePath)

    # Define headers (optional, but good practice)
    # The Content-Type might need to be adjusted based on what the server expects.
    # 'image/gif' is standard for GIF files.
    $headers = @{
        "Content-Type" = "image/gif"
    }

    # Perform the POST request
    # -Method Post: Specifies the HTTP POST method.
    # -Uri: The target URL for the upload.
    # -Body: The content to be sent in the request body (the GIF file bytes).
    # -Headers: Custom headers for the request.
    # -ContentType: This is set explicitly in $headers, but can also be here.
    #               If -ContentType is used directly, it overrides the header.
    #               For raw binary, it's often better to control it via -Headers.
    $response = Invoke-RestMethod -Method Post -Uri $TargetUrl -Body $fileContent -Headers $headers -TimeoutSec 30

    Write-Host "Upload successful!"
    Write-Host "Server Response:"
    # Output the response from the server.
    # The type of $response depends on what the server returns (e.g., JSON, text).
    $response | Out-Host

}
catch {
    Write-Error "An error occurred during the upload process:"
    Write-Error $_.Exception.Message
    Write-Error "Details: $($_.Exception | Format-List -Force | Out-String)"
}
