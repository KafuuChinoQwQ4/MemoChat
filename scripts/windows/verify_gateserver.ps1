$ErrorActionPreference = "Continue"
$pid = 21780
$proc = Get-Process -Id $pid -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "GateServer process PID $pid is running."
    Write-Host "Process name: $($proc.ProcessName)"
    Write-Host "Start time: $($proc.StartTime)"
} else {
    Write-Host "GateServer process PID $pid is NOT running."
}
# Check all ports
Write-Host "`nAll ports with 8080:"
netstat -ano | findstr ":8080"
