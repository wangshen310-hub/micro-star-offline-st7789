param(
    [string]$Port,
    [string]$Latitude,
    [string]$Longitude
)

$ErrorActionPreference = "Stop"
$invariant = [System.Globalization.CultureInfo]::InvariantCulture

if ([string]::IsNullOrWhiteSpace($Latitude)) { $Latitude = Read-Host "Latitude (-90..90)" }
if ([string]::IsNullOrWhiteSpace($Longitude)) { $Longitude = Read-Host "Longitude (-180..180)" }

[double]$lat = 0
[double]$lon = 0
$latValid = [double]::TryParse($Latitude, [Globalization.NumberStyles]::Float, $invariant, [ref]$lat)
$lonValid = [double]::TryParse($Longitude, [Globalization.NumberStyles]::Float, $invariant, [ref]$lon)
if (-not $latValid -or -not $lonValid -or $lat -lt -90 -or $lat -gt 90 -or $lon -lt -180 -or $lon -gt 180) {
    throw "Latitude/longitude must be decimal degrees: latitude -90..90, longitude -180..180."
}

if ([string]::IsNullOrWhiteSpace($Port)) {
    $ports = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
    if ($ports.Count -eq 0) { throw "No serial port found. Connect the device with a data-capable USB cable." }
    $Port = $ports[0]
}

$serial = [System.IO.Ports.SerialPort]::new($Port, 115200, [IO.Ports.Parity]::None, 8, [IO.Ports.StopBits]::One)
$serial.ReadTimeout = 300
try {
    $serial.Open()
    Start-Sleep -Milliseconds 500
    $serial.DiscardInBuffer()
    $command = [string]::Format($invariant, "SETLOC {0:F6} {1:F6}", $lat, $lon)
    $serial.WriteLine($command)
    $deadline = [DateTime]::UtcNow.AddSeconds(4)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $line = $serial.ReadLine().Trim()
            if ($line -like "LOCATION_OK*") {
                Write-Host "Location saved: $line"
                exit 0
            }
            if ($line -like "LOCATION_ERROR*") { throw $line }
        } catch [TimeoutException] {
        }
    }
    throw "No LOCATION_OK response from $Port."
} finally {
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
}
