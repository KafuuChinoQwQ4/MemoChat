$ErrorActionPreference = "Stop"
$exePath = "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\GateServer.exe"
$configPath = "D:\MemoChat-Qml-Drogon\server\GateServer\config.ini"
$pidFile = "D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\runtime\pids\GateServer.pid"

# Start the process and capture stdout/stderr
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

$stdout = $process.StandardOutput.ReadToEnd()
$stderr = $process.StandardError.ReadToEnd()

$process.WaitForExit()

Write-Host "Exit code: $($process.ExitCode)"
Write-Host "STDOUT:"
Write-Host $stdout
Write-Host "STDERR:"
Write-Host $stderr
