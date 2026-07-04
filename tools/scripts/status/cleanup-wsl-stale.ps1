[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [string]$Distro = $(if ($env:MEMOCHAT_WSL_DISTRO) { $env:MEMOCHAT_WSL_DISTRO } else { 'archlinux' }),

    [Alias('ProjectRoot')]
    [string]$ProjectPath = $(if ($env:MEMOCHAT_WSL_PROJECT_ROOT) { $env:MEMOCHAT_WSL_PROJECT_ROOT } else { '/root/code/MemoChat' }),

    [ValidateRange(1, 10080)]
    [int]$MaxAgeMinutes = 15,

    [switch]$Stop,

    [switch]$IncludeMcp
)

$ErrorActionPreference = 'Stop'

$protectedPatterns = @(
    'mcp',
    'modelcontextprotocol',
    'context7',
    'filesystem',
    'postgres',
    'postgresql',
    'redis',
    'neo4j',
    'qdrant',
    'redpanda',
    'mongo',
    'mongodb',
    'rabbitmq',
    'minio',
    'prometheus',
    'grafana',
    'loki',
    'tempo',
    'cadvisor',
    'influx',
    'ollama',
    'docker compose',
    'docker-compose',
    'dockerd'
)

$transientPatterns = @(
    'docker run --rm',
    '/usr/bin/git -C',
    'git diff --check',
    'git status --short',
    'date -Is',
    '/bin/echo',
    '/bin/true',
    'python -m unittest',
    'python3 -m unittest',
    'python -m compileall',
    'python3 -m compileall',
    'cmake --build',
    'ninja -C',
    'chmod +x',
    '/usr/bin/chmod +x',
    'sed -n',
    'rg ',
    'grep ',
    'cat ',
    'ls ',
    'pwd'
)

function Test-CommandLineContains {
    param(
        [AllowNull()]
        [string]$CommandLine,

        [string[]]$Patterns
    )

    if ([string]::IsNullOrWhiteSpace($CommandLine)) {
        return $false
    }

    foreach ($pattern in $Patterns) {
        if ($CommandLine.IndexOf($pattern, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
            return $true
        }
    }

    return $false
}

function Test-DistroMatch {
    param(
        [AllowNull()]
        [string]$CommandLine,

        [string]$ExpectedDistro
    )

    if ([string]::IsNullOrWhiteSpace($CommandLine)) {
        return $false
    }

    $escaped = [regex]::Escape($ExpectedDistro)
    return $CommandLine -match "(?i)(^|\s)(-d|--distribution)\s+`"?$escaped`"?(?=\s|$)"
}

function Test-ProjectMatch {
    param(
        [AllowNull()]
        [string]$CommandLine,

        [string]$ExpectedProjectPath
    )

    if ([string]::IsNullOrWhiteSpace($CommandLine) -or [string]::IsNullOrWhiteSpace($ExpectedProjectPath)) {
        return $false
    }

    $candidates = New-Object System.Collections.Generic.List[string]
    $candidates.Add($ExpectedProjectPath)
    $candidates.Add($ExpectedProjectPath.Replace('\', '/'))

    foreach ($candidate in ($candidates | Select-Object -Unique)) {
        if (-not [string]::IsNullOrWhiteSpace($candidate) -and
            $CommandLine.IndexOf($candidate, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
            return $true
        }
    }

    return $false
}

function Get-CreationDate {
    param($Process)

    if ($Process.CreationDate -is [datetime]) {
        return [datetime]$Process.CreationDate
    }

    return [System.Management.ManagementDateTimeConverter]::ToDateTime([string]$Process.CreationDate)
}

$now = Get-Date
$allWslProcesses = Get-CimInstance Win32_Process -Filter "name='wsl.exe'" |
    Where-Object { $_.Name -ieq 'wsl.exe' }

$rows = foreach ($process in $allWslProcesses) {
    $commandLine = if ($process.CommandLine) { [string]$process.CommandLine } else { '' }
    $createdAt = Get-CreationDate $process
    $ageMinutes = [math]::Round(($now - $createdAt).TotalMinutes, 1)

    $isDistro = Test-DistroMatch -CommandLine $commandLine -ExpectedDistro $Distro
    $isProject = Test-ProjectMatch -CommandLine $commandLine -ExpectedProjectPath $ProjectPath
    $isProtected = (Test-CommandLineContains -CommandLine $commandLine -Patterns $protectedPatterns)
    $isTransient = (Test-CommandLineContains -CommandLine $commandLine -Patterns $transientPatterns)
    $isOldEnough = $ageMinutes -ge $MaxAgeMinutes

    $reason = if (-not $isDistro) {
        'different-distro'
    } elseif (-not $isProject) {
        'outside-project'
    } elseif ($isProtected -and -not $IncludeMcp) {
        'protected-helper'
    } elseif (-not $isTransient) {
        'not-transient'
    } elseif (-not $isOldEnough) {
        'too-young'
    } else {
        'candidate'
    }

    [pscustomobject]@{
        Pid = [int]$process.ProcessId
        AgeMinutes = $ageMinutes
        Reason = $reason
        WillStop = [bool]($Stop -and $reason -eq 'candidate')
        CommandLine = $commandLine
    }
}

$candidates = @($rows | Where-Object { $_.Reason -eq 'candidate' })

Write-Host "[INFO] MemoChat WSL stale wrapper cleanup"
Write-Host "[INFO] Distro: $Distro"
Write-Host "[INFO] ProjectPath: $ProjectPath"
Write-Host "[INFO] MaxAgeMinutes: $MaxAgeMinutes"
Write-Host "[INFO] Mode: $(if ($Stop) { 'stop candidates' } else { 'dry-run report only' })"
Write-Host "[INFO] MCP/helper protection: $(if ($IncludeMcp) { 'disabled by -IncludeMcp' } else { 'enabled' })"

if ($rows) {
    $rows |
        Sort-Object Reason, AgeMinutes -Descending |
        Select-Object Pid, AgeMinutes, Reason, WillStop, CommandLine |
        Format-Table -AutoSize -Wrap
} else {
    Write-Host "[OK] No wsl.exe wrapper processes found."
}

if (-not $candidates) {
    Write-Host "[OK] No stale project WSL wrapper candidates found."
    return
}

if (-not $Stop) {
    Write-Host "[DRY-RUN] $($candidates.Count) candidate(s) found. Re-run with -Stop to stop them, or add -WhatIf first."
    return
}

foreach ($candidate in $candidates) {
    $target = "wsl.exe PID $($candidate.Pid)"
    $action = "Stop stale MemoChat project wrapper process"
    if ($PSCmdlet.ShouldProcess($target, $action)) {
        Stop-Process -Id $candidate.Pid -Force -ErrorAction Stop
        Write-Host "[OK] Stopped $target"
    }
}
