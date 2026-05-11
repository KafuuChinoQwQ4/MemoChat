param(
    [string]$BaseUrl = "http://127.0.0.1",
    [string]$ProbeRoute = "/__nginx_failover_probe",
    [int[]]$BackendPorts = @(8080, 8084),
    [switch]$StopBackend,
    [int]$StopBackendPort = 8080,
    [switch]$NoRestore,
    [int]$ProbeCount = 4,
    [int]$FaultProbeCount = 12,
    [int]$ProbeDelayMs = 500,
    [int]$RestoreTimeoutSec = 45,
    [int]$TimeoutSec = 5,
    [int[]]$AcceptableStatusCodes = @(200, 400, 401, 404, 405, 429),
    [switch]$AllowGatewayErrors,
    [string]$NginxContainerName = "memochat-nginx-lb",
    [int]$DockerLogTail = 160,
    [string]$RestoreScript = "",
    [string]$RuntimeServicesDir = "infra/Memo_ops/runtime/services",
    [string]$ServiceRunner = "tools/scripts/status/run-service-console.ps1"
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Net.Http

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptRoot)
$DockerCli = Join-Path $ScriptRoot "docker\arch-docker.ps1"

function Join-Url {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Base,
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $normalizedBase = $Base.TrimEnd("/")
    if ($Path.StartsWith("/")) {
        return "$normalizedBase$Path"
    }
    return "$normalizedBase/$Path"
}

function Get-ListeningProcessIds {
    param([Parameter(Mandatory = $true)][int]$Port)

    $connections = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
    if (-not $connections) {
        return @()
    }

    return @($connections |
        Where-Object { $_.OwningProcess -and $_.OwningProcess -gt 0 } |
        Select-Object -ExpandProperty OwningProcess -Unique)
}

function Test-PortListening {
    param([Parameter(Mandatory = $true)][int]$Port)

    return ((Get-ListeningProcessIds -Port $Port).Count -gt 0)
}

function Write-PortState {
    param([int[]]$Ports)

    Write-Host "=== GateServer port state ==="
    $allListening = $true
    foreach ($port in $Ports) {
        $pids = Get-ListeningProcessIds -Port $port
        if ($pids.Count -eq 0) {
            Write-Host "Port ${port}: not listening"
            $allListening = $false
            continue
        }

        $processText = @()
        foreach ($processId in $pids) {
            $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
            if ($process) {
                $processText += "$processId/$($process.ProcessName)"
            } else {
                $processText += "$processId/<exited>"
            }
        }
        Write-Host "Port ${port}: listening by $($processText -join ', ')"
    }

    return $allListening
}

function Invoke-Probe {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Uri,
        [Parameter(Mandatory = $true)]
        [int[]]$ExpectedStatusCodes,
        [Parameter(Mandatory = $true)]
        [int]$RequestNumber,
        [Parameter(Mandatory = $true)]
        [int]$RequestTimeoutSec
    )

    $client = [System.Net.Http.HttpClient]::new()
    $client.Timeout = [TimeSpan]::FromSeconds($RequestTimeoutSec)
    $request = [System.Net.Http.HttpRequestMessage]::new([System.Net.Http.HttpMethod]::Get, $Uri)
    $request.Headers.TryAddWithoutValidation("X-MemoChat-Smoke", "nginx-failover") | Out-Null
    $request.Headers.TryAddWithoutValidation("X-MemoChat-Smoke-Seq", [string]$RequestNumber) | Out-Null

    $startedAt = Get-Date
    try {
        $response = $client.SendAsync($request).GetAwaiter().GetResult()
        try {
            $statusCode = [int]$response.StatusCode
            $elapsedMs = [int]((Get-Date) - $startedAt).TotalMilliseconds
            $accepted = $ExpectedStatusCodes -contains $statusCode
            return [pscustomobject]@{
                Number    = $RequestNumber
                Status    = $statusCode
                Accepted  = $accepted
                Error     = $null
                ElapsedMs = $elapsedMs
            }
        } finally {
            $response.Dispose()
        }
    } catch {
        $elapsedMs = [int]((Get-Date) - $startedAt).TotalMilliseconds
        return [pscustomobject]@{
            Number    = $RequestNumber
            Status    = $null
            Accepted  = $false
            Error     = $_.Exception.Message
            ElapsedMs = $elapsedMs
        }
    } finally {
        $request.Dispose()
        $client.Dispose()
    }
}

function Invoke-BoundedProbes {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Uri,
        [Parameter(Mandatory = $true)]
        [int]$Count,
        [Parameter(Mandatory = $true)]
        [int[]]$ExpectedStatusCodes,
        [Parameter(Mandatory = $true)]
        [int]$DelayMs,
        [Parameter(Mandatory = $true)]
        [int]$RequestTimeoutSec
    )

    Write-Host "=== Nginx probes ==="
    Write-Host "Target: GET $Uri"
    Write-Host "Count: $Count"
    Write-Host "Accepted status codes: $($ExpectedStatusCodes -join ', ')"

    $results = @()
    for ($i = 1; $i -le $Count; $i++) {
        $result = Invoke-Probe -Uri $Uri -ExpectedStatusCodes $ExpectedStatusCodes -RequestNumber $i -RequestTimeoutSec $RequestTimeoutSec
        $results += $result

        if ($result.Error) {
            Write-Host ("Probe {0}: ERROR after {1}ms: {2}" -f $result.Number, $result.ElapsedMs, $result.Error)
        } else {
            $marker = if ($result.Accepted) { "accepted" } else { "unexpected" }
            Write-Host ("Probe {0}: HTTP {1} ({2}, {3}ms)" -f $result.Number, $result.Status, $marker, $result.ElapsedMs)
        }

        if ($i -lt $Count -and $DelayMs -gt 0) {
            Start-Sleep -Milliseconds $DelayMs
        }
    }

    return $results
}

function Show-NginxUpstreamLogSummary {
    param(
        [string]$ContainerName,
        [int]$Tail,
        [datetime]$Since
    )

    Write-Host "=== Recent Nginx upstream_addr values ==="
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $dockerArgs = @("logs", "--tail", [string]$Tail)
        if ($Since) {
            $dockerArgs += @("--since", $Since.ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ"))
        }
        $dockerArgs += $ContainerName
        $logs = @(& $DockerCli @dockerArgs 2>&1 | ForEach-Object { $_.ToString() })
        $dockerExitCode = $LASTEXITCODE
    } catch {
        Write-Host "Docker log check unavailable: $($_.Exception.Message)"
        return $false
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($dockerExitCode -ne 0) {
        Write-Host "Docker log check unavailable: docker logs exited with $dockerExitCode."
        return $false
    }

    $rows = New-Object 'System.Collections.Generic.List[object]'
    foreach ($line in $logs) {
        if (-not $line) {
            continue
        }

        try {
            $entry = $line | ConvertFrom-Json -ErrorAction Stop
            if ($entry.upstream_addr) {
                $rows.Add([pscustomobject]@{
                    UpstreamAddr         = [string]$entry.upstream_addr
                    UpstreamStatus       = [string]$entry.upstream_status
                    UpstreamResponseTime = [string]$entry.upstream_response_time
                    Status               = [string]$entry.status
                    Request              = [string]$entry.request
                })
            }
        } catch {
            $match = [regex]::Match($line, '"upstream_addr"\s*:\s*"([^"]+)"')
            if ($match.Success) {
                $rows.Add([pscustomobject]@{
                    UpstreamAddr         = $match.Groups[1].Value
                    UpstreamStatus       = ""
                    UpstreamResponseTime = ""
                    Status               = ""
                    Request              = ""
                })
            }
        }
    }

    if ($rows.Count -eq 0) {
        if ($Since) {
            Write-Host "No upstream_addr values found in the last $Tail log lines since $($Since.ToString('s'))."
        } else {
            Write-Host "No upstream_addr values found in the last $Tail log lines."
        }
        return $false
    }

    $rows |
        Select-Object -Last 20 |
        Format-Table -AutoSize UpstreamAddr, UpstreamStatus, UpstreamResponseTime, Status, Request |
        Out-String -Width 220 |
        ForEach-Object { Write-Host $_ }

    $rows |
        Group-Object UpstreamAddr |
        Sort-Object Name |
        ForEach-Object { Write-Host ("{0}: {1}" -f $_.Name, $_.Count) }

    return $true
}

function Stop-ListeningProcessForPort {
    param([Parameter(Mandatory = $true)][int]$Port)

    $pids = Get-ListeningProcessIds -Port $Port
    if ($pids.Count -eq 0) {
        throw "No listening process found on port $Port."
    }

    if ($pids.Count -gt 1) {
        throw "Refusing to stop port $Port because multiple listening process IDs were found: $($pids -join ', ')."
    }

    $pidToStop = [int]$pids[0]
    $process = Get-Process -Id $pidToStop -ErrorAction Stop
    if ($process.ProcessName -ne "GateServer") {
        throw "Refusing to stop PID $pidToStop on port $Port because it is '$($process.ProcessName)', not GateServer."
    }

    Write-Host "Stopping process on port $Port only: PID=$pidToStop Name=$($process.ProcessName)"
    Stop-Process -Id $pidToStop -Force -ErrorAction Stop
    return [pscustomobject]@{
        Port        = $Port
        ProcessId   = $pidToStop
        ProcessName = $process.ProcessName
    }
}

function Wait-PortState {
    param(
        [Parameter(Mandatory = $true)]
        [int]$Port,
        [Parameter(Mandatory = $true)]
        [bool]$ShouldListen,
        [Parameter(Mandatory = $true)]
        [int]$TimeoutSec
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    do {
        $isListening = Test-PortListening -Port $Port
        if ($isListening -eq $ShouldListen) {
            return $true
        }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)

    return $false
}

function Invoke-Restore {
    param(
        [string]$ScriptPath,
        [Parameter(Mandatory = $true)]
        [int]$StoppedPort,
        [Parameter(Mandatory = $true)]
        [int]$TimeoutSec,
        [Parameter(Mandatory = $true)]
        [string]$ServicesDir,
        [Parameter(Mandatory = $true)]
        [string]$RunnerPath
    )

    if ([string]::IsNullOrWhiteSpace($ScriptPath)) {
        return Invoke-GateServerRestore -StoppedPort $StoppedPort -TimeoutSec $TimeoutSec -ServicesDir $ServicesDir -RunnerPath $RunnerPath
    }

    if ([System.IO.Path]::IsPathRooted($ScriptPath)) {
        $candidateScript = $ScriptPath
    } else {
        $candidateScript = Join-Path -Path $ProjectRoot -ChildPath $ScriptPath
    }

    $resolvedScript = Resolve-Path -Path $candidateScript -ErrorAction SilentlyContinue
    if (-not $resolvedScript) {
        Write-Host "Restore script not found: $ScriptPath"
        return $false
    }

    Write-Host "=== Restore ==="
    Write-Host "Running: $resolvedScript"
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        & $resolvedScript.Path
        $restoreExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($restoreExitCode -ne 0) {
        Write-Host "Restore script exited with $restoreExitCode."
    }

    $restored = Wait-PortState -Port $StoppedPort -ShouldListen $true -TimeoutSec $TimeoutSec
    if ($restored) {
        Write-Host "Port $StoppedPort is listening again."
    } else {
        Write-Host "Port $StoppedPort did not become listening within $TimeoutSec seconds."
    }

    return ($restoreExitCode -eq 0 -and $restored)
}

function Invoke-GateServerRestore {
    param(
        [Parameter(Mandatory = $true)]
        [int]$StoppedPort,
        [Parameter(Mandatory = $true)]
        [int]$TimeoutSec,
        [Parameter(Mandatory = $true)]
        [string]$ServicesDir,
        [Parameter(Mandatory = $true)]
        [string]$RunnerPath
    )

    if ($StoppedPort -eq 8080) {
        $serviceName = "GateServer-1"
        $serviceFolder = "GateServer1"
    } elseif ($StoppedPort -eq 8084) {
        $serviceName = "GateServer-2"
        $serviceFolder = "GateServer2"
    } else {
        Write-Host "No targeted GateServer restore mapping for port $StoppedPort."
        return $false
    }

    $resolvedServicesDir = if ([System.IO.Path]::IsPathRooted($ServicesDir)) {
        $ServicesDir
    } else {
        Join-Path -Path $ProjectRoot -ChildPath $ServicesDir
    }
    $serviceDir = Join-Path -Path $resolvedServicesDir -ChildPath $serviceFolder
    $exePath = Join-Path -Path $serviceDir -ChildPath "GateServer.exe"

    $resolvedRunner = if ([System.IO.Path]::IsPathRooted($RunnerPath)) {
        $RunnerPath
    } else {
        Join-Path -Path $ProjectRoot -ChildPath $RunnerPath
    }

    if (-not (Test-Path -LiteralPath $exePath)) {
        Write-Host "Restore executable not found: $exePath"
        return $false
    }
    if (-not (Test-Path -LiteralPath $resolvedRunner)) {
        Write-Host "Restore runner not found: $resolvedRunner"
        return $false
    }

    $logDir = Join-Path -Path $ProjectRoot -ChildPath "infra/Memo_ops/artifacts/logs/services"
    New-Item -ItemType Directory -Force -Path $logDir | Out-Null
    $logName = $serviceName.Replace("-", "_")
    $logOut = Join-Path -Path $logDir -ChildPath "${logName}_out.log"
    $logErr = Join-Path -Path $logDir -ChildPath "${logName}_err.log"

    Write-Host "=== Restore ==="
    Write-Host "Starting targeted $serviceName for port $StoppedPort."
    Start-Process `
        -FilePath "powershell.exe" `
        -ArgumentList @(
            "-NoProfile",
            "-ExecutionPolicy", "Bypass",
            "-NoExit",
            "-File", $resolvedRunner,
            "-ServiceDir", $serviceDir,
            "-ExeName", "GateServer.exe",
            "-ServiceName", $serviceName,
            "-ConfigName", "config.ini",
            "-LogOut", $logOut,
            "-LogErr", $logErr
        ) `
        -WindowStyle Hidden | Out-Null

    $restored = Wait-PortState -Port $StoppedPort -ShouldListen $true -TimeoutSec $TimeoutSec
    if ($restored) {
        Write-Host "Port $StoppedPort is listening again."
    } else {
        Write-Host "Port $StoppedPort did not become listening within $TimeoutSec seconds."
    }

    return $restored
}

if ($ProbeCount -lt 1) {
    Write-Host "ProbeCount must be at least 1."
    exit 2
}

if ($FaultProbeCount -lt 1) {
    Write-Host "FaultProbeCount must be at least 1."
    exit 2
}

if ($TimeoutSec -lt 1) {
    Write-Host "TimeoutSec must be at least 1."
    exit 2
}

if ($StopBackendPort -notin $BackendPorts) {
    Write-Host "StopBackendPort $StopBackendPort is not in BackendPorts: $($BackendPorts -join ', ')"
    exit 2
}

if ($AllowGatewayErrors) {
    $AcceptableStatusCodes = @($AcceptableStatusCodes + @(502, 503, 504) | Select-Object -Unique)
}

$probeUri = Join-Url -Base $BaseUrl -Path $ProbeRoute
$ok = $true
$stoppedProcess = $null
$probeWindowStartedAt = Get-Date

Write-Host "=== Nginx failover smoke ==="
Write-Host "Mode: $(if ($StopBackend) { 'fault-injection' } else { 'non-destructive' })"
Write-Host "Backend ports: $($BackendPorts -join ', ')"
Write-Host "StopBackendPort: $StopBackendPort"
Write-Host "Restore: $(if ($NoRestore) { 'disabled' } else { 'enabled' })"
Write-Host "Gateway errors accepted: $(if ($AllowGatewayErrors) { 'yes' } else { 'no' })"

try {
    $portsOk = Write-PortState -Ports $BackendPorts
    if (-not $portsOk -and -not $StopBackend) {
        $ok = $false
    }

    if (-not $StopBackend) {
        $probeResults = Invoke-BoundedProbes `
            -Uri $probeUri `
            -Count $ProbeCount `
            -ExpectedStatusCodes $AcceptableStatusCodes `
            -DelayMs $ProbeDelayMs `
            -RequestTimeoutSec $TimeoutSec

        $ok = $ok -and (($probeResults | Where-Object { $_.Accepted }).Count -eq $probeResults.Count)
        $logsOk = Show-NginxUpstreamLogSummary -ContainerName $NginxContainerName -Tail $DockerLogTail -Since $probeWindowStartedAt
        $ok = $ok -and $logsOk
    } else {
        $stoppedProcess = Stop-ListeningProcessForPort -Port $StopBackendPort

        $portDown = Wait-PortState -Port $StopBackendPort -ShouldListen $false -TimeoutSec 10
        if (-not $portDown) {
            Write-Host "Port $StopBackendPort is still listening after stop."
            $ok = $false
        } else {
            Write-Host "Port $StopBackendPort is down."
        }

        $remainingPorts = @($BackendPorts | Where-Object { $_ -ne $StopBackendPort })
        foreach ($port in $remainingPorts) {
            if (-not (Test-PortListening -Port $port)) {
                Write-Host "Remaining backend port $port is not listening."
                $ok = $false
            }
        }

        $probeResults = Invoke-BoundedProbes `
            -Uri $probeUri `
            -Count $FaultProbeCount `
            -ExpectedStatusCodes $AcceptableStatusCodes `
            -DelayMs $ProbeDelayMs `
            -RequestTimeoutSec $TimeoutSec

        $acceptedCount = ($probeResults | Where-Object { $_.Accepted }).Count
        Write-Host "Accepted probes: $acceptedCount/$($probeResults.Count)"
        $ok = $ok -and ($acceptedCount -eq $probeResults.Count)
        $logsOk = Show-NginxUpstreamLogSummary -ContainerName $NginxContainerName -Tail $DockerLogTail -Since $probeWindowStartedAt
        $ok = $ok -and $logsOk
    }
} catch {
    Write-Host "Failover smoke failed: $($_.Exception.Message)"
    if ($_.InvocationInfo -and $_.InvocationInfo.PositionMessage) {
        Write-Host $_.InvocationInfo.PositionMessage
    }
    if ($_.ScriptStackTrace) {
        Write-Host $_.ScriptStackTrace
    }
    $ok = $false
} finally {
    if ($StopBackend -and $stoppedProcess) {
        if ($NoRestore) {
            if ($RestoreScript) {
                Write-Host "Restore skipped by -NoRestore. To restore, run: $RestoreScript"
            } else {
                Write-Host "Restore skipped by -NoRestore. To restore, run: tools/scripts/status/start-all-services.bat"
            }
        } else {
            $restoreOk = Invoke-Restore -ScriptPath $RestoreScript -StoppedPort $StopBackendPort -TimeoutSec $RestoreTimeoutSec -ServicesDir $RuntimeServicesDir -RunnerPath $ServiceRunner
            $ok = $ok -and $restoreOk
        }
    }
}

if (-not $ok) {
    exit 1
}
