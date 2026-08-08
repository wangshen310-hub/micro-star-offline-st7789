param(
    [int]$PollSeconds = 2,
    [int]$RetrySeconds = 10
)

$ErrorActionPreference = "Continue"
$installDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$syncScript = Join-Path $installDir "StarTimeSync.ps1"
$logPath = Join-Path $installDir "StarTimeAutoSync.log"

function Write-AutoSyncLog {
    param([string]$Message)
    $line = "$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')  $Message"
    [System.IO.File]::AppendAllText($logPath, $line + [Environment]::NewLine)
}

$createdNew = $false
$mutex = [System.Threading.Mutex]::new($true, "Local\MicroStarAutoTimeSync", [ref]$createdNew)
if (-not $createdNew) {
    exit 0
}

$ports = @{}
Write-AutoSyncLog "Watcher started."

try {
    while ($true) {
        $currentPorts = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)

        foreach ($knownPort in @($ports.Keys)) {
            if ($currentPorts -notcontains $knownPort) {
                Write-AutoSyncLog "$knownPort disconnected."
                $ports.Remove($knownPort)
            }
        }

        foreach ($port in $currentPorts) {
            if (-not $ports.ContainsKey($port)) {
                $ports[$port] = @{
                    Synced = $false
                    NextAttempt = [datetime]::MinValue
                }
                Write-AutoSyncLog "$port connected."
            }

            $state = $ports[$port]
            if (-not $state.Synced -and [datetime]::UtcNow -ge $state.NextAttempt) {
                $output = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $syncScript -Port $port 2>&1 | Out-String
                if ($LASTEXITCODE -eq 0) {
                    $state.Synced = $true
                    Write-AutoSyncLog "$port synchronized successfully."
                }
                else {
                    $state.NextAttempt = [datetime]::UtcNow.AddSeconds($RetrySeconds)
                    $summary = ($output -replace '\s+', ' ').Trim()
                    Write-AutoSyncLog "$port not identified; retry scheduled. $summary"
                }
            }
        }

        Start-Sleep -Seconds ([Math]::Max(1, $PollSeconds))
    }
}
finally {
    Write-AutoSyncLog "Watcher stopped."
    if ($createdNew) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}