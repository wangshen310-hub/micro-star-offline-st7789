param(
    [string]$Port = ""
)

$ErrorActionPreference = "Stop"

function Read-LinesUntil {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [datetime]$Deadline
    )

    $lines = @()
    while ([datetime]::UtcNow -lt $Deadline) {
        try {
            $line = $Serial.ReadLine().Trim()
            if ($line) {
                $lines += $line
                Write-Host $line
            }
        }
        catch [System.TimeoutException] {
        }
    }
    return $lines
}

$candidates = if ($Port) {
    @($Port)
}
else {
    @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
}

if ($candidates.Count -eq 0) {
    throw "No serial ports were found. Connect the Micro Star device by USB."
}

foreach ($candidate in $candidates) {
    $serial = $null
    try {
        Write-Host "Checking $candidate ..."
        $serial = [System.IO.Ports.SerialPort]::new(
            $candidate,
            115200,
            [System.IO.Ports.Parity]::None,
            8,
            [System.IO.Ports.StopBits]::One
        )
        $serial.NewLine = "`n"
        $serial.ReadTimeout = 250
        $serial.WriteTimeout = 1000
        $serial.DtrEnable = $false
        $serial.RtsEnable = $false
        $serial.Open()

        Start-Sleep -Milliseconds 1800
        $serial.DiscardInBuffer()

        $identified = $false
        for ($attempt = 0; $attempt -lt 5 -and -not $identified; $attempt++) {
            $serial.WriteLine("PING")
            $lines = Read-LinesUntil -Serial $serial -Deadline ([datetime]::UtcNow.AddMilliseconds(700))
            $identified = $lines -contains "MICROSTAR"
        }

        if (-not $identified) {
            continue
        }

        $epoch = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
        $offsetMinutes = [int][TimeZoneInfo]::Local.GetUtcOffset([DateTime]::Now).TotalMinutes
        $serial.WriteLine("SETTIME $epoch $offsetMinutes")

        $reply = Read-LinesUntil -Serial $serial -Deadline ([datetime]::UtcNow.AddSeconds(4))
        $success = $reply | Where-Object { $_ -like "TIME_OK *" }
        if (-not $success) {
            throw "The device was found, but it did not confirm the time."
        }

        Write-Host ""
        Write-Host "Time synchronization succeeded on $candidate." -ForegroundColor Green
        Write-Host "The device is now running completely offline."
        exit 0
    }
    catch {
        Write-Warning "$candidate : $($_.Exception.Message)"
    }
    finally {
        if ($null -ne $serial -and $serial.IsOpen) {
            $serial.Close()
        }
        if ($null -ne $serial) {
            $serial.Dispose()
        }
    }
}

throw "No Micro Star device responded. Close other serial monitors and try again."
