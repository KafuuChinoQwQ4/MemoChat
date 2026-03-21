$ErrorActionPreference = "Continue"

# Check if PID 12692 is running
$proc = Get-Process -Id 12692 -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "GateServer process PID 12692 is running."
    Write-Host "Process name: $($proc.ProcessName)"
    Write-Host "Start time: $($proc.StartTime)"
} else {
    Write-Host "GateServer process PID 12692 is NOT running."
}

# Check all processes with GateServer in name
Write-Host "`nAll GateServer processes:"
Get-Process | Where-Object { $_.ProcessName -like "*Gate*" } | Format-Table Id, ProcessName, StartTime -AutoSize

# Check netstat for port 8080 listening
Write-Host "`nPort 8080 status:"
netstat -ano | findstr "8080"

# Check if any process is listening on port 8080
Write-Host "`nLooking for processes listening on port 8080:"
$listening = netstat -ano | findstr "LISTENING"
$listening | ForEach-Object {
    if ($_ -match ":8080\s+.*LISTENING\s+(\d+)") {
        $pid = $matches[1]
        Write-Host "PID $pid is listening on port 8080"
        $p = Get-Process -Id $pid -ErrorAction SilentlyContinue
        if ($p) {
            Write-Host "  Process: $($p.ProcessName)"
        }
    }
}
