$ErrorActionPreference = "Continue"

# Check if PID 25512 is running
$proc = Get-Process -Id 25512 -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "GateServer process PID 25512 is running."
    Write-Host "Process name: $($proc.ProcessName)"
    Write-Host "Start time: $($proc.StartTime)"
} else {
    Write-Host "GateServer process PID 25512 is NOT running."
}

# Check all processes
Write-Host "`nAll processes:"
Get-Process | Where-Object { $_.ProcessName -like "*Gate*" } | Format-Table Id, ProcessName, StartTime -AutoSize

# Check all LISTENING ports
Write-Host "`nAll ports with 8080:"
netstat -ano | findstr ":8080"
