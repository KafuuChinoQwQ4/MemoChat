$ErrorActionPreference = 'Stop'
$instances = @(
    @{Dir="D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\chatserver1"; Name="chatserver1"; WorkerId="1"},
    @{Dir="D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\chatserver2"; Name="chatserver2"; WorkerId="2"},
    @{Dir="D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\chatserver3"; Name="chatserver3"; WorkerId="3"},
    @{Dir="D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\chatserver4"; Name="chatserver4"; WorkerId="4"}
)
foreach ($inst in $instances) {
    $conf = Join-Path $inst.Dir "config.ini"
    if (-not (Test-Path $conf)) {
        Write-Host "[SKIP] $conf not found"
        continue
    }
    $c = [IO.File]::ReadAllText($conf)
    $before = if ($c -match '(?m)^Name\s*=\s*(\S+)') { $matches[1] } else { "?" }
    $beforeWid = if ($c -match '(?m)^WorkerId\s*=\s*(\S+)') { $matches[1] } else { "?" }
    Write-Host "[$($inst.Name)] BEFORE: Name=$before, WorkerId=$beforeWid"
    $c = $c -replace '(?m)^Name\s*=\s*\S+',"Name=$($inst.Name)"
    $c = $c -replace '(?m)^WorkerId\s*=\s*\S+',"WorkerId=$($inst.WorkerId)"
    [IO.File]::WriteAllText($conf, $c)
    Write-Host "[$($inst.Name)] PATCHED: Name=$($inst.Name), WorkerId=$($inst.WorkerId)"
}
Write-Host "Done."
