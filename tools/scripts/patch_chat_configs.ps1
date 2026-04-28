$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$runtimeRoot = Join-Path $repoRoot 'infra\Memo_ops\runtime\services'

$instances = 1..6 | ForEach-Object {
    @{
        Dir = Join-Path $runtimeRoot "chatserver$_"
        Name = "chatserver$_"
        WorkerId = "$_"
    }
}

function Set-IniValue {
    param(
        [string[]]$Lines,
        [string]$Section,
        [string]$Key,
        [string]$Value
    )

    $out = New-Object System.Collections.Generic.List[string]
    $current = ''
    $inserted = $false
    $sectionSeen = $false

    foreach ($line in $Lines) {
        if ($line -cmatch '^\[([^\]]+)\]$') {
            if ($sectionSeen -and -not $inserted) {
                $out.Add("$Key=$Value")
                $inserted = $true
            }
            $current = $matches[1]
            $sectionSeen = ($current -eq $Section)
            $out.Add($line)
            continue
        }

        if ($current -eq $Section -and $line -cmatch ('^' + [regex]::Escape($Key) + '=')) {
            if (-not $inserted) {
                $out.Add("$Key=$Value")
                $inserted = $true
            }
            continue
        }

        $out.Add($line)
    }

    if ($sectionSeen -and -not $inserted) {
        $out.Add("$Key=$Value")
    }

    return [string[]]$out.ToArray()
}

foreach ($inst in $instances) {
    $conf = Join-Path $inst.Dir 'config.ini'
    if (-not (Test-Path -LiteralPath $conf)) {
        Write-Host "[SKIP] $conf not found"
        continue
    }

    $lines = [System.IO.File]::ReadAllLines($conf)
    $lines = Set-IniValue $lines 'SelfServer' 'Name' $inst.Name
    $lines = Set-IniValue $lines 'Snowflake' 'DatacenterId' '1'
    $lines = Set-IniValue $lines 'Snowflake' 'WorkerId' $inst.WorkerId
    $lines = Set-IniValue $lines 'Kafka' 'ClientId' "memochat-$($inst.Name)"
    $lines = Set-IniValue $lines 'Log' 'Dir' "../../artifacts/logs/services/$($inst.Name)"
    [System.IO.File]::WriteAllLines($conf, $lines)

    Write-Host "[$($inst.Name)] patched WorkerId=$($inst.WorkerId)"
}

Write-Host 'Done.'
