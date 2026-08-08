$ErrorActionPreference = "Stop"
$installDir = Join-Path $env:LOCALAPPDATA "MicroStar"
$startupFile = Join-Path ([Environment]::GetFolderPath("Startup")) "MicroStarAutoTimeSync.cmd"

if (Test-Path -LiteralPath $startupFile) {
    Remove-Item -LiteralPath $startupFile -Force
}
Write-Host "Automatic startup has been removed. The current watcher exits at Windows sign-out." -ForegroundColor Green
Write-Host "Installed files remain at $installDir so the log can be inspected."