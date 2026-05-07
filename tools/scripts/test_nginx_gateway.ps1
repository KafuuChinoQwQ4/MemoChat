param(
    [string]$BaseUrl = "http://127.0.0.1",
    [switch]$Login,
    [switch]$ProbePolicyRoutes,
    [switch]$ProbeResponseHeaders,
    [switch]$ShowHeaders,
    [string]$RequestId,
    [string]$TraceId,
    [switch]$GenerateRequestId,
    [switch]$GenerateTraceId,
    [switch]$GenerateIds,
    [switch]$CheckDockerLogs,
    [string]$NginxContainerName = "memochat-nginx-lb",
    [int]$DockerLogTail = 100,
    [string]$Email = "test",
    [string]$Password = "test",
    [string]$ClientVersion = "3.0.0",
    [int]$TimeoutSec = 5
)

$ErrorActionPreference = "Stop"

function Get-ResponseHeaderValue {
    param(
        $Headers,
        [string]$Name
    )

    if (-not $Headers) {
        return $null
    }

    if ($Headers -is [System.Net.WebHeaderCollection]) {
        return $Headers.Get($Name)
    }

    return $Headers[$Name]
}

function Write-ResponseHeaders {
    param($Headers)

    if (-not $Headers) {
        return
    }

    $names = @()
    if ($Headers -is [System.Net.WebHeaderCollection]) {
        $names = $Headers.AllKeys
    } else {
        $names = $Headers.Keys
    }

    foreach ($key in $names) {
        Write-Host "Header: $key=$(Get-ResponseHeaderValue -Headers $Headers -Name $key)"
    }
}

function New-SmokeCorrelationId {
    param([string]$Prefix)

    return "$Prefix-$([guid]::NewGuid().ToString('N'))"
}

function New-CorrelationHeaders {
    param(
        [string]$ResolvedRequestId,
        [string]$ResolvedTraceId
    )

    $headers = @{}
    if ($ResolvedRequestId) {
        $headers["X-Request-Id"] = $ResolvedRequestId
    }
    if ($ResolvedTraceId) {
        $headers["X-Trace-Id"] = $ResolvedTraceId
    }
    return $headers
}

function Merge-Headers {
    param(
        [hashtable]$BaseHeaders = @{},
        [hashtable]$ExtraHeaders = @{}
    )

    $merged = @{}
    foreach ($key in $BaseHeaders.Keys) {
        $merged[$key] = $BaseHeaders[$key]
    }
    foreach ($key in $ExtraHeaders.Keys) {
        $merged[$key] = $ExtraHeaders[$key]
    }
    return $merged
}

function Test-NginxDockerLogs {
    param(
        [string]$ContainerName,
        [int]$Tail,
        [string]$ExpectedRequestId,
        [string]$ExpectedTraceId
    )

    if (-not $ExpectedRequestId -and -not $ExpectedTraceId) {
        Write-Host "Docker log check skipped: no request or trace ID was set."
        return $true
    }

    Write-Host "=== Nginx docker logs ==="
    $logs = $null
    $dockerExitCode = $null
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $logs = & docker logs --tail $Tail $ContainerName 2>&1
        $dockerExitCode = $LASTEXITCODE
    } catch {
        Write-Host "Docker log check unavailable: $($_.Exception.Message)"
        return $true
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($dockerExitCode -ne 0) {
        Write-Host "Docker log check unavailable: docker logs exited with $dockerExitCode."
        return $true
    }

    $logText = $logs -join "`n"
    $matched = $true

    if ($ExpectedRequestId) {
        if ($logText -match [regex]::Escape($ExpectedRequestId)) {
            Write-Host "Found request ID in recent Nginx logs: $ExpectedRequestId"
        } else {
            Write-Host "Missing request ID in recent Nginx logs: $ExpectedRequestId"
            $matched = $false
        }
    }

    if ($ExpectedTraceId) {
        if ($logText -match [regex]::Escape($ExpectedTraceId)) {
            Write-Host "Found trace ID in recent Nginx logs: $ExpectedTraceId"
        } else {
            Write-Host "Missing trace ID in recent Nginx logs: $ExpectedTraceId"
            $matched = $false
        }
    }

    return $matched
}

function Invoke-GatewayRequest {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [Parameter(Mandatory = $true)]
        [string]$Uri,
        [string]$Method = "GET",
        [string]$Body,
        [hashtable]$Headers = @{},
        [int[]]$ExpectedStatusCodes = @(200),
        [hashtable]$ExpectedHeaders = @{}
    )

    Write-Host "=== $Name ==="
    $statusCode = $null
    $responseHeaders = $null
    $content = $null

    try {
        $args = @{
            Uri             = $Uri
            Method          = $Method
            TimeoutSec      = $TimeoutSec
            UseBasicParsing = $true
        }

        if ($Headers.Count -gt 0) {
            $args.Headers = $Headers
        }

        if ($Body) {
            $args.ContentType = "application/json"
            $args.Body = $Body
        }

        $response = Invoke-WebRequest @args
        $statusCode = [int]$response.StatusCode
        $responseHeaders = $response.Headers
        $content = $response.Content
    } catch {
        $webResponse = $_.Exception.Response
        if ($webResponse -and $webResponse.StatusCode) {
            $statusCode = [int]$webResponse.StatusCode
            $responseHeaders = $webResponse.Headers

            try {
                $stream = $webResponse.GetResponseStream()
                if ($stream) {
                    $reader = New-Object System.IO.StreamReader($stream)
                    $content = $reader.ReadToEnd()
                    $reader.Dispose()
                }
            } catch {
                $content = $null
            }
        } else {
            Write-Host "Error: $($_.Exception.Message)"
            return $false
        }
    }

    Write-Host "Status: $statusCode"
    if ($ShowHeaders -or $ProbeResponseHeaders) {
        Write-ResponseHeaders -Headers $responseHeaders
    }

    if ($content) {
        if ($content -is [byte[]]) {
            $content = [System.Text.Encoding]::UTF8.GetString($content)
        }
        Write-Host "Body: $content"
    }

    if ($ExpectedStatusCodes -notcontains $statusCode) {
        Write-Host "Unexpected status. Expected one of: $($ExpectedStatusCodes -join ', ')"
        return $false
    }

    foreach ($headerName in $ExpectedHeaders.Keys) {
        $actual = Get-ResponseHeaderValue -Headers $responseHeaders -Name $headerName
        $expected = $ExpectedHeaders[$headerName]
        if (-not $actual) {
            Write-Host "Missing expected header: $headerName"
            return $false
        }
        if ($expected -and ($actual -ne $expected)) {
            Write-Host "Unexpected header $headerName. Expected '$expected', got '$actual'"
            return $false
        }
    }

    return $true
}

$normalizedBaseUrl = $BaseUrl.TrimEnd("/")

$resolvedRequestId = $RequestId
$resolvedTraceId = $TraceId
if (($GenerateIds -or $GenerateRequestId) -and -not $resolvedRequestId) {
    $resolvedRequestId = New-SmokeCorrelationId -Prefix "nginx-smoke-request"
}
if (($GenerateIds -or $GenerateTraceId) -and -not $resolvedTraceId) {
    $resolvedTraceId = New-SmokeCorrelationId -Prefix "nginx-smoke-trace"
}

$correlationHeaders = New-CorrelationHeaders -ResolvedRequestId $resolvedRequestId -ResolvedTraceId $resolvedTraceId
if ($resolvedRequestId) {
    Write-Host "RequestId: $resolvedRequestId"
}
if ($resolvedTraceId) {
    Write-Host "TraceId: $resolvedTraceId"
}

$ok = Invoke-GatewayRequest -Name "Nginx health" -Uri "$normalizedBaseUrl/health" -Headers $correlationHeaders

if ($Login) {
    $payload = @{
        email      = $Email
        passwd     = $Password
        client_ver = $ClientVersion
    } | ConvertTo-Json -Compress

    $loginOk = Invoke-GatewayRequest -Name "Nginx user_login" -Uri "$normalizedBaseUrl/user_login" -Method "POST" -Body $payload -Headers $correlationHeaders
    $ok = $ok -and $loginOk
}

if ($ProbePolicyRoutes -or $ProbeResponseHeaders) {
    $probeId = New-SmokeCorrelationId -Prefix "nginx-smoke"
    $probeHeaders = Merge-Headers -BaseHeaders @{
        "X-Request-Id" = $probeId
        "X-Trace-Id"   = $probeId
    } -ExtraHeaders $correlationHeaders
    $safeRouteStatuses = @(200, 400, 401, 404, 405, 429, 502, 503, 504)

    if ($ProbePolicyRoutes) {
        $authOk = Invoke-GatewayRequest `
            -Name "Auth policy route user_login" `
            -Uri "$normalizedBaseUrl/user_login" `
            -Method "POST" `
            -Body "{}" `
            -Headers $probeHeaders `
            -ExpectedStatusCodes $safeRouteStatuses

        $mediaUploadOk = Invoke-GatewayRequest `
            -Name "Media upload policy route" `
            -Uri "$normalizedBaseUrl/upload_media_status" `
            -Headers $probeHeaders `
            -ExpectedStatusCodes $safeRouteStatuses

        $mediaDownloadOk = Invoke-GatewayRequest `
            -Name "Media download policy route" `
            -Uri "$normalizedBaseUrl/media/download" `
            -Headers $probeHeaders `
            -ExpectedStatusCodes $safeRouteStatuses

        $aiOk = Invoke-GatewayRequest `
            -Name "AI stream policy route" `
            -Uri "$normalizedBaseUrl/ai/chat/stream" `
            -Headers $probeHeaders `
            -ExpectedStatusCodes $safeRouteStatuses

        $ok = $ok -and $authOk -and $mediaUploadOk -and $mediaDownloadOk -and $aiOk
    } elseif ($ProbeResponseHeaders) {
        $headersOk = Invoke-GatewayRequest `
            -Name "AI stream response headers" `
            -Uri "$normalizedBaseUrl/ai/chat/stream" `
            -Headers $probeHeaders `
            -ExpectedStatusCodes $safeRouteStatuses `
            -ExpectedHeaders @{ "X-Accel-Buffering" = "no" }

        $ok = $ok -and $headersOk
    }
}

if ($CheckDockerLogs) {
    if ($resolvedRequestId -or $resolvedTraceId) {
        $logProbeOk = Invoke-GatewayRequest `
            -Name "Nginx log correlation probe" `
            -Uri "$normalizedBaseUrl/__nginx_smoke_correlation" `
            -Headers $correlationHeaders `
            -ExpectedStatusCodes @(200, 400, 401, 404, 405, 429, 502, 503, 504)

        $ok = $ok -and $logProbeOk
    }

    $logsOk = Test-NginxDockerLogs `
        -ContainerName $NginxContainerName `
        -Tail $DockerLogTail `
        -ExpectedRequestId $resolvedRequestId `
        -ExpectedTraceId $resolvedTraceId

    $ok = $ok -and $logsOk
}

if (-not $ok) {
    exit 1
}
