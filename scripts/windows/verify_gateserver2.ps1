$ErrorActionPreference = "Continue"
$gatePid = 21780
$proc = Get-Process -Id $gatePid -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "GateServer process PID $gatePid is running."
    Write-Host "Process name: $($proc.ProcessName)"
    Write-Host "Start time: $($proc.StartTime)"
} else {
    Write-Host "GateServer process PID $gatePid is NOT running."
}
# Check all ports
Write-Host "`nAll ports with 8080:"
netstat -ano | findstr ":8080"
