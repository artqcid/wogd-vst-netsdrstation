# Download WebView2 SDK Headers for self-contained VST3 plugin
# This script downloads the WebView2 SDK headers needed for compilation
# and ensures the runtime is bundled with the VST3 plugin.

param(
    [string]$SdkVersion = "1.0.1510.0",
    [string]$TargetDir = "C:\Users\marku\Documents\GitHub\thirdParty\WebView2SDK"
)

$ErrorActionPreference = "Stop"

Write-Host "Downloading WebView2 SDK v$SdkVersion..."

# Create target directory
if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
}

# Download SDK zip
$downloadUrl = "https://github.com/MicrosoftEdge/WebView2SDK/releases/download/v$SdkVersion/Microsoft.WebView2.SDK.$SdkVersion.zip"
$tempZip = "$env:TEMP\WebView2SDK.zip"

try {
    Invoke-WebRequest -Uri $downloadUrl -OutFile $tempZip -UseBasicParsing
    Write-Host "Downloaded SDK to $tempZip"
    
    # Extract SDK
    Expand-Archive -Path $tempZip -DestinationPath $TargetDir -Force
    Write-Host "Extracted SDK to $TargetDir"
    
    # Verify headers exist
    $headerPath = Join-Path $TargetDir "include\WebView2.h"
    if (Test-Path $headerPath) {
        Write-Host "SUCCESS: WebView2.h found at $headerPath"
    } else {
        Write-Warning "WebView2.h not found - SDK structure may differ"
    }
    
    # Verify runtime exists
    $runtimePath = Join-Path $TargetDir "runtimes\x64"
    if (Test-Path $runtimePath) {
        Write-Host "SUCCESS: Runtime found at $runtimePath"
    } else {
        Write-Warning "Runtime not found at $runtimePath"
    }
}
finally {
    # Cleanup
    if (Test-Path $tempZip) {
        Remove-Item $tempZip -Force
    }
}

Write-Host "WebView2 SDK setup complete!"