param(
    [string]$ComposeFile = "infra/deploy/local/docker-compose.yml",
    [string]$EnvoyContainerName = "memochat-envoy-gateway",
    [string]$PrometheusBaseUrl = "http://127.0.0.1:9090",
    [string]$CurlImage = "alpine:3.20",
    [switch]$SkipPrometheus,
    [int]$TimeoutSec = 10
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptRoot)
$DockerCli = Join-Path $ScriptRoot "docker\arch-docker.ps1"

function Resolve-ProjectPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path -Path $ProjectRoot -ChildPath $Path
}

function Invoke-ContainerNetworkHttpText {
    param(
        [Parameter(Mandatory = $true)][string]$ContainerName,
        [Parameter(Mandatory = $true)][string]$Uri,
        [Parameter(Mandatory = $true)][string]$Image,
        [Parameter(Mandatory = $true)][int]$RequestTimeoutSec
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $wgetCommand = "wget -q -O - -T $RequestTimeoutSec '$Uri'"
        $dockerArgs = @(
            "run", "--rm",
            "--network", "container:$ContainerName",
            $Image,
            "sh", "-ec", $wgetCommand
        )
        $output = @(& $DockerCli @dockerArgs 2>&1 | ForEach-Object { $_.ToString() })
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Text     = ($output -join "`n")
    }
}

function Invoke-HttpText {
    param(
        [Parameter(Mandatory = $true)][string]$Uri,
        [Parameter(Mandatory = $true)][int]$RequestTimeoutSec
    )

    try {
        $response = Invoke-WebRequest -Uri $Uri -UseBasicParsing -TimeoutSec $RequestTimeoutSec
        return [pscustomobject]@{
            Ok         = $true
            StatusCode = [int]$response.StatusCode
            Text       = [string]$response.Content
            Error      = $null
        }
    } catch {
        return [pscustomobject]@{
            Ok         = $false
            StatusCode = $null
            Text       = ""
            Error      = $_.Exception.Message
        }
    }
}

function Test-PrometheusQuery {
    param(
        [Parameter(Mandatory = $true)][string]$BaseUrl,
        [Parameter(Mandatory = $true)][string]$Query,
        [Parameter(Mandatory = $true)][int]$RequestTimeoutSec
    )

    $encodedQuery = [System.Uri]::EscapeDataString($Query)
    $uri = "$($BaseUrl.TrimEnd('/'))/api/v1/query?query=$encodedQuery"
    $result = Invoke-HttpText -Uri $uri -RequestTimeoutSec $RequestTimeoutSec
    if (-not $result.Ok) {
        Write-Host "Prometheus query failed: $($result.Error)"
        return $false
    }

    try {
        $json = $result.Text | ConvertFrom-Json -ErrorAction Stop
    } catch {
        Write-Host "Prometheus query returned invalid JSON: $($_.Exception.Message)"
        return $false
    }

    if ($json.status -ne "success") {
        Write-Host "Prometheus query status was not success: $($json.status)"
        return $false
    }

    $rows = @($json.data.result)
    if ($rows.Count -eq 0) {
        Write-Host "Prometheus query returned no samples: $Query"
        return $false
    }

    Write-Host "Prometheus query returned $($rows.Count) sample(s): $Query"
    foreach ($row in $rows | Select-Object -First 5) {
        $value = if ($row.value -and $row.value.Count -ge 2) { $row.value[1] } else { "<missing>" }
        $job = $row.metric.job
        $instance = $row.metric.instance
        Write-Host "  job=$job instance=$instance value=$value"
    }

    return $true
}

if ($TimeoutSec -lt 1) {
    Write-Host "TimeoutSec must be at least 1."
    exit 2
}

$composePath = Resolve-ProjectPath -Path $ComposeFile
$ok = $true

Write-Host "=== Envoy metrics smoke ==="
Write-Host "Compose: $composePath"
Write-Host "Envoy container: $EnvoyContainerName"
Write-Host "Curl image: $CurlImage"

Write-Host "=== Compose config ==="
$previousErrorActionPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = "Continue"
    & $DockerCli compose -f $composePath config --quiet
    $composeExitCode = $LASTEXITCODE
} finally {
    $ErrorActionPreference = $previousErrorActionPreference
}
if ($composeExitCode -eq 0) {
    Write-Host "Compose config OK."
} else {
    Write-Host "Compose config failed with exit code $composeExitCode."
    $ok = $false
}

Write-Host "=== Envoy admin stats ==="
$stats = Invoke-ContainerNetworkHttpText -ContainerName $EnvoyContainerName -Uri "http://127.0.0.1:9901/stats/prometheus" -Image $CurlImage -RequestTimeoutSec $TimeoutSec
$metricLines = @($stats.Text -split "`n" | Where-Object { $_ -match "^(envoy_server_live|envoy_http_downstream_rq_total|envoy_cluster_upstream_cx_connect_fail)" })
$metricLines | Select-Object -First 20 | ForEach-Object { Write-Host $_ }
if ($stats.ExitCode -ne 0 -or $stats.Text -notmatch "envoy_server_live") {
    Write-Host "Envoy admin stats check failed."
    $ok = $false
}

if (-not $SkipPrometheus) {
    Write-Host "=== Prometheus ==="
    $promReady = Invoke-HttpText -Uri "$($PrometheusBaseUrl.TrimEnd('/'))/-/ready" -RequestTimeoutSec $TimeoutSec
    if ($promReady.Ok) {
        Write-Host "Prometheus ready: HTTP $($promReady.StatusCode)"
        $promOk = Test-PrometheusQuery -BaseUrl $PrometheusBaseUrl -Query 'envoy_server_live{job="envoy_gateway"}' -RequestTimeoutSec $TimeoutSec
        $ok = $ok -and $promOk
    } else {
        Write-Host "Prometheus ready check failed: $($promReady.Error)"
        $ok = $false
    }
} else {
    Write-Host "Prometheus checks skipped by -SkipPrometheus."
}

if (-not $ok) {
    exit 1
}
