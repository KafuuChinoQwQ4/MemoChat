$ErrorActionPreference = "SilentlyContinue"

$targets = Get-CimInstance Win32_Process | Where-Object {
    ($_.Name -like "python*.exe" -and $_.CommandLine -match "Memo_ops\.server\.ops_server\.main|Memo_ops\.server\.ops_collector\.main") -or
    ($_.Name -eq "MemoOpsQml.exe")
}

foreach ($process in $targets) {
    Stop-Process -Id $process.ProcessId -Force
}
