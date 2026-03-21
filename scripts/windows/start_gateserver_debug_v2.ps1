$ErrorActionPreference = "Stop"

# Try to start the process with redirected output
$exePath = "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\GateServer.exe"
$configPath = "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\config.ini"
$pidFile = "D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\runtime\pids\GateServer.pid"

$pinfo = New-Object System.Diagnostics.ProcessStartInfo
$pinfo.FileName = $exePath
$pinfo.Arguments = "--config=""$configPath"""
$pinfo.WorkingDirectory = "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer"
$pinfo.RedirectStandardError = $true
$pinfo.RedirectStandardOutput = $true
$pinfo.UseShellExecute = $false
$pinfo.CreateNoWindow = $true

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $pinfo
$process.Start() | Out-Null

$stdoutTask = $process.StandardOutput.ReadToEndAsync()
$stderrTask = $process.StandardError.ReadToEndAsync()

$process.WaitForExit(5000) | Out-Null

if ($process.HasExited) {
    Write-Host "Exit code: $($process.ExitCode)"
    Write-Host "STDOUT:"
    $stdoutTask.Result
    Write-Host "STDERR:"
    $stderrTask.Result
} else {
    Write-Host "Process is running with PID: $($process.Id)"
    $process.Id | Out-File -FilePath $pidFile -Encoding ASCII
    Write-Host "PID file updated."
}
