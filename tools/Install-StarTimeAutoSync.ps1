$ErrorActionPreference = "Stop"

$sourceDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$installDir = Join-Path $env:LOCALAPPDATA "MicroStar"
$startupDir = [Environment]::GetFolderPath("Startup")
$startupFile = Join-Path $startupDir "MicroStarAutoTimeSync.cmd"

New-Item -ItemType Directory -Path $installDir -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $sourceDir "StarTimeSync.ps1") -Destination (Join-Path $installDir "StarTimeSync.ps1") -Force
Copy-Item -LiteralPath (Join-Path $sourceDir "StarTimeAutoSync.ps1") -Destination (Join-Path $installDir "StarTimeAutoSync.ps1") -Force

$launcher = '@echo off' + [Environment]::NewLine +
    'start "" /min powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "%LOCALAPPDATA%\MicroStar\StarTimeAutoSync.ps1"' + [Environment]::NewLine
[System.IO.File]::WriteAllText($startupFile, $launcher, [System.Text.Encoding]::ASCII)

Start-Process powershell.exe -ArgumentList @(
    "-NoProfile",
    "-WindowStyle", "Hidden",
    "-ExecutionPolicy", "Bypass",
    "-File", (Join-Path $installDir "StarTimeAutoSync.ps1")
) -WindowStyle Hidden

Write-Host "Micro Star automatic time synchronization is installed." -ForegroundColor Green
Write-Host "It will start automatically when you sign in to Windows."
Write-Host "Every USB reconnect will trigger time synchronization."
Write-Host "Log: $(Join-Path $installDir 'StarTimeAutoSync.log')"