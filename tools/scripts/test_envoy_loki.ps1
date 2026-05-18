param(
    [string]$BaseUrl = "http://127.0.0.1",
    [string]$LokiUrl = "http://127.0.0.1:3100",
    [string]$ProbePath = "/__envoy_loki_probe",
    [string]$RequestId,
    [string]$TraceId,
    [string]$AccessLogPath = "\\wsl.localhost\archlinux\data\docker-data\memochat\envoy\logs\access.json",
    [switch]$SkipAccessLogFileCheck,
    [int]$TimeoutSec = 5,
    [int]$FileWaitSec = 10,
    [int]$LokiWaitSec = 45,
    [int]$RetryIntervalSec = 3,
    [int]$AccessLogTail = 300,
    [int]$LokiLimit = 20
)

$ErrorActionPreference = "Stop"
$script:UnixEpochUtc = [DateTime]::Parse("1970-01-01T00:00:00.0000000Z").ToUniversalTime()

function New-SmokeCorrelationId {
    param([string]$Prefix)

    return "$Prefix-$([guid]::NewGuid().ToString('N'))"
}

function ConvertTo-LogQLStringLiteral {
    param([string]$Text)

    $escaped = $Text.Replace('\', '\\').Replace('"', '\"')
    return '"' + $escaped + '"'
}

function Get-UnixTimeNanoseconds {
    param([DateTime]$UtcDate)

    $ticks = [int64](($UtcDate.ToUniversalTime() - $script:UnixEpochUtc).Ticks)
    return ($ticks * 100)
}

function New-LokiQueryUrl {
    param(
        [string]$Endpoint,
        [string]$Expression,
        [DateTime]$StartUtc,
        [DateTime]$EndUtc,
        [int]$Limit
    )

    $normalizedEndpoint = $Endpoint.TrimEnd("/")
    $encodedExpression = [System.Uri]::EscapeDataString($Expression)
    $startNs = Get-UnixTimeNanoseconds -UtcDate $StartUtc
    $endNs = Get-UnixTimeNanoseconds -UtcDate $EndUtc

    return "$normalizedEndpoint/loki/api/v1/query_range?query=$encodedExpression&start=$startNs&end=$endNs&limit=$Limit&direction=backward"
}

function Read-WebErrorBody {
    param($Response)

    if (-not $Response) {
        return $null
    }

    try {
        $stream = $Response.GetResponseStream()
        if (-not $stream) {
            return $null
        }

        $reader = New-Object System.IO.StreamReader($stream)
        try {
            return $reader.ReadToEnd()
        } finally {
            $reader.Dispose()
        }
    } catch {
        return $null
    }
}

function Invoke-EnvoyRequestProbe {
    param(
        [string]$Endpoint,
        [string]$PathAndQuery,
        [string]$ExpectedRequestId,
        [string]$ExpectedTraceId,
        [int]$RequestTimeoutSec,
        [int]$ExpectedStatus = -1,
        [string]$DisplayPathAndQuery
    )

    $normalizedEndpoint = $Endpoint.TrimEnd("/")
    $normalizedPath = $PathAndQuery
    if (-not $normalizedPath.StartsWith("/")) {
        $normalizedPath = "/$normalizedPath"
    }
    $uri = "$normalizedEndpoint$normalizedPath"
    $displayPath = $DisplayPathAndQuery
    if (-not $displayPath) {
        $displayPath = $normalizedPath
    }
    if (-not $displayPath.StartsWith("/")) {
        $displayPath = "/$displayPath"
    }
    $displayUri = "$normalizedEndpoint$displayPath"
    $headers = @{
        "X-Request-Id" = $ExpectedRequestId
        "X-Trace-Id"   = $ExpectedTraceId
    }

    Write-Host "ProbeUrl: $displayUri"

    try {
        $response = Invoke-WebRequest -Uri $uri -Method GET -Headers $headers -TimeoutSec $RequestTimeoutSec -UseBasicParsing
        $statusCode = [int]$response.StatusCode
        Write-Host "ProbeStatus: $statusCode"

        if ($response.Content) {
            Write-Host "ProbeBody: $($response.Content)"
        }

        if ($ExpectedStatus -ge 0 -and $statusCode -ne $ExpectedStatus) {
            Write-Host "Probe failed: expected status $ExpectedStatus."
            return $false
        }

        return $true
    } catch {
        $webResponse = $_.Exception.Response
        if ($webResponse -and $webResponse.StatusCode) {
            $statusCode = [int]$webResponse.StatusCode
            Write-Host "ProbeStatus: $statusCode"
            $body = Read-WebErrorBody -Response $webResponse
            if ($body) {
                Write-Host "ProbeBody: $body"
            }
            if ($ExpectedStatus -ge 0) {
                Write-Host "Probe failed: expected status $ExpectedStatus."
                return $false
            }
            return $true
        } else {
            Write-Host "Probe failed: $($_.Exception.Message)"
        }
        return $false
    }
}

function Invoke-EnvoyHealthProbe {
    param(
        [string]$Endpoint,
        [string]$ExpectedRequestId,
        [string]$ExpectedTraceId,
        [int]$RequestTimeoutSec
    )

    return Invoke-EnvoyRequestProbe `
        -Endpoint $Endpoint `
        -PathAndQuery $ProbePath `
        -ExpectedRequestId $ExpectedRequestId `
        -ExpectedTraceId $ExpectedTraceId `
        -RequestTimeoutSec $RequestTimeoutSec `
        -ExpectedStatus 200
}

function Test-AccessLogFileForRequest {
    param(
        [string]$Path,
        [string]$ExpectedRequestId,
        [int]$WaitSec,
        [int]$IntervalSec,
        [int]$Tail
    )

    if ($WaitSec -lt 0) {
        $WaitSec = 0
    }
    if ($IntervalSec -lt 1) {
        $IntervalSec = 1
    }
    if ($Tail -lt 1) {
        $Tail = 1
    }

    Write-Host "AccessLogPath: $Path"
    $deadline = [DateTime]::UtcNow.AddSeconds($WaitSec)
    $lastError = $null

    do {
        if (Test-Path -LiteralPath $Path) {
            try {
                $matches = @(Get-Content -LiteralPath $Path -Tail $Tail -ErrorAction Stop | Select-String -SimpleMatch $ExpectedRequestId)
                if ($matches.Count -gt 0) {
                    Write-Host "AccessLogFileMatched: true"
                    Write-Host "AccessLogFileMatchCount: $($matches.Count)"
                    return $true
                }
                $lastError = "request ID was not present in the last $Tail access log lines"
            } catch {
                $lastError = $_.Exception.Message
            }
        } else {
            $lastError = "file does not exist"
        }

        if ([DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Seconds $IntervalSec
        }
    } while ([DateTime]::UtcNow -lt $deadline)

    Write-Host "AccessLogFileMatched: false"
    if ($lastError) {
        Write-Host "AccessLogFileDiagnostic: $lastError"
    }
    return $false
}

function Get-StatusCountsText {
    param([hashtable]$StatusCounts)

    if (-not $StatusCounts -or $StatusCounts.Count -eq 0) {
        return "unavailable"
    }

    $parts = @()
    foreach ($key in ($StatusCounts.Keys | Sort-Object)) {
        $parts += "$key=$($StatusCounts[$key])"
    }

    return ($parts -join ", ")
}

function Add-StatusCountFromLine {
    param(
        [hashtable]$StatusCounts,
        [string]$Line
    )

    if (-not $Line) {
        return
    }

    $statusValue = $null
    try {
        $json = $Line | ConvertFrom-Json -ErrorAction Stop
        if ($null -ne $json.status) {
            $statusValue = [string]$json.status
        }
    } catch {
        $statusValue = $null
    }

    if (-not $statusValue -and $Line -match '"status"\s*:\s*"?([0-9]{3})"?') {
        $statusValue = $Matches[1]
    }

    if ($statusValue) {
        if ($StatusCounts.ContainsKey($statusValue)) {
            $StatusCounts[$statusValue] = $StatusCounts[$statusValue] + 1
        } else {
            $StatusCounts[$statusValue] = 1
        }
    }
}

function Invoke-LokiRequestIdQuery {
    param(
        [string]$QueryUrl,
        [int]$RequestTimeoutSec
    )

    $response = Invoke-RestMethod -Uri $QueryUrl -Method GET -TimeoutSec $RequestTimeoutSec -UseBasicParsing
    $streams = @()
    if ($response -and $response.data -and $null -ne $response.data.result) {
        $streams = @($response.data.result)
    }

    $entryCount = 0
    $statusCounts = @{}
    $lines = @()
    $jsonEntries = @()

    foreach ($stream in $streams) {
        if (-not $stream -or -not $stream.values) {
            continue
        }

        foreach ($entry in $stream.values) {
            if (-not $entry -or $entry.Count -lt 2) {
                continue
            }

            $entryCount += 1
            $line = [string]$entry[1]
            $lines += $line
            Add-StatusCountFromLine -StatusCounts $statusCounts -Line $line

            $jsonEntry = ConvertTo-EnvoyLogObject -Line $line
            if ($jsonEntry) {
                $jsonEntries += @($jsonEntry)
            }
        }
    }

    return [pscustomobject]@{
        ApiStatus    = $response.status
        ResultType   = $response.data.resultType
        StreamCount  = $streams.Count
        EntryCount   = $entryCount
        StatusCounts = $statusCounts
        Lines        = $lines
        JsonEntries  = $jsonEntries
    }
}

function ConvertTo-EnvoyLogObject {
    param([string]$Line)

    if (-not $Line) {
        return $null
    }

    try {
        $parsed = $Line | ConvertFrom-Json -ErrorAction Stop

        if ($null -ne $parsed.route_family -or $null -ne $parsed.uri_path) {
            return $parsed
        }

        if ($parsed.attributes -and ($null -ne $parsed.attributes.route_family -or $null -ne $parsed.attributes.uri_path)) {
            return $parsed.attributes
        }

        if ($parsed.body) {
            try {
                $body = [string]$parsed.body | ConvertFrom-Json -ErrorAction Stop
                if ($null -ne $body.route_family -or $null -ne $body.uri_path) {
                    return $body
                }
            } catch {
            }
        }
    } catch {
    }

    return $null
}

function Wait-LokiRequestIdQuery {
    param(
        [string]$Endpoint,
        [string]$Expression,
        [DateTime]$StartUtc,
        [int]$WaitSec,
        [int]$IntervalSec,
        [int]$Limit,
        [int]$RequestTimeoutSec,
        [string]$QueryLabel
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($WaitSec)
    $attempt = 0
    $lastError = $null

    do {
        $attempt += 1
        $queryEndUtc = [DateTime]::UtcNow.AddSeconds(10)
        $queryUrl = New-LokiQueryUrl -Endpoint $Endpoint -Expression $Expression -StartUtc $StartUtc -EndUtc $queryEndUtc -Limit $Limit

        if ($attempt -eq 1) {
            Write-Host "${QueryLabel}LokiQueryUrl: $queryUrl"
        }

        try {
            $result = Invoke-LokiRequestIdQuery -QueryUrl $queryUrl -RequestTimeoutSec $RequestTimeoutSec
            Write-Host "${QueryLabel}LokiAttempt: $attempt ApiStatus=$($result.ApiStatus) ResultType=$($result.ResultType) Streams=$($result.StreamCount) Entries=$($result.EntryCount)"
            Write-Host "${QueryLabel}LokiStatusCounts: $(Get-StatusCountsText -StatusCounts $result.StatusCounts)"

            if ($result.EntryCount -gt 0) {
                return [pscustomobject]@{
                    Matched   = $true
                    Result    = $result
                    LastError = $null
                }
            }
        } catch {
            $lastError = $_.Exception.Message
            Write-Host "${QueryLabel}LokiAttempt: $attempt Error=$lastError"
        }

        if ([DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Seconds $IntervalSec
        }
    } while ([DateTime]::UtcNow -lt $deadline)

    return [pscustomobject]@{
        Matched   = $false
        Result    = $null
        LastError = $lastError
    }
}

function Test-LokiClassificationFields {
    param(
        [object[]]$JsonEntries,
        [string]$ExpectedRouteFamily,
        [string]$ExpectedStatusClass
    )

    foreach ($entry in $JsonEntries) {
        if (-not $entry) {
            continue
        }

        if ([string]$entry.route_family -eq $ExpectedRouteFamily -and [string]$entry.status_class -eq $ExpectedStatusClass) {
            Write-Host "LokiClassificationMatched: true route_family=$ExpectedRouteFamily status_class=$ExpectedStatusClass"
            return $true
        }
    }

    Write-Host "LokiClassificationMatched: false expected route_family=$ExpectedRouteFamily status_class=$ExpectedStatusClass"
    return $false
}

function Test-LokiRedaction {
    param(
        [string[]]$Lines,
        [object[]]$JsonEntries,
        [string]$Secret
    )

    $rawTokenLines = @($Lines | Where-Object { $_ -match 'token=' -or $_ -like "*$Secret*" })
    if ($rawTokenLines.Count -gt 0) {
        Write-Host "LokiRedactionMatched: false raw token material was present in $($rawTokenLines.Count) matched line(s)"
        return $false
    }

    $mediaEntries = @($JsonEntries | Where-Object {
        [string]$_.route_family -eq "media" -and
        [string]$_.query_present -eq "true" -and
        [string]$_.uri_path -eq "/media/download"
    })

    if ($mediaEntries.Count -lt 1) {
        Write-Host "LokiRedactionMatched: false expected sanitized media entry with query_present=true"
        return $false
    }

    Write-Host "LokiRedactionMatched: true"
    return $true
}

if (-not $RequestId) {
    $RequestId = New-SmokeCorrelationId -Prefix "envoy-loki-request"
}
if (-not $TraceId) {
    $TraceId = New-SmokeCorrelationId -Prefix "envoy-loki-trace"
}
if ($TimeoutSec -lt 1) {
    $TimeoutSec = 1
}
if ($LokiWaitSec -lt 1) {
    $LokiWaitSec = 1
}
if ($RetryIntervalSec -lt 1) {
    $RetryIntervalSec = 1
}
if ($LokiLimit -lt 1) {
    $LokiLimit = 1
}

$queryStartUtc = [DateTime]::UtcNow.AddSeconds(-30)
$logQLRequestId = ConvertTo-LogQLStringLiteral -Text $RequestId
$lokiExpression = '{service="memochat-envoy-gateway"} |= ' + $logQLRequestId
$redactionRequestId = New-SmokeCorrelationId -Prefix "envoy-loki-redact-request"
$redactionTraceId = New-SmokeCorrelationId -Prefix "envoy-loki-redact-trace"
$redactionSecret = "envoy-loki-secret-$([guid]::NewGuid().ToString('N'))"
$redactionPath = "/media/download?token=$redactionSecret&asset=loki-smoke"
$logQLRedactionRequestId = ConvertTo-LogQLStringLiteral -Text $redactionRequestId
$redactionLokiExpression = '{service="memochat-envoy-gateway"} |= ' + $logQLRedactionRequestId

Write-Host "RequestId: $RequestId"
Write-Host "TraceId: $TraceId"
Write-Host "LokiExpression: $lokiExpression"
Write-Host "RedactionRequestId: $redactionRequestId"
Write-Host "RedactionTraceId: $redactionTraceId"
Write-Host "RedactionPath: /media/download?token=<redacted>&asset=loki-smoke"
Write-Host "RedactionLokiExpression: $redactionLokiExpression"

$ok = Invoke-EnvoyHealthProbe -Endpoint $BaseUrl -ExpectedRequestId $RequestId -ExpectedTraceId $TraceId -RequestTimeoutSec $TimeoutSec
$redactionProbeOk = Invoke-EnvoyRequestProbe `
    -Endpoint $BaseUrl `
    -PathAndQuery $redactionPath `
    -ExpectedRequestId $redactionRequestId `
    -ExpectedTraceId $redactionTraceId `
    -RequestTimeoutSec $TimeoutSec `
    -ExpectedStatus -1 `
    -DisplayPathAndQuery "/media/download?token=<redacted>&asset=loki-smoke"

$ok = $ok -and $redactionProbeOk

if (-not $SkipAccessLogFileCheck) {
    $fileOk = Test-AccessLogFileForRequest `
        -Path $AccessLogPath `
        -ExpectedRequestId $RequestId `
        -WaitSec $FileWaitSec `
        -IntervalSec $RetryIntervalSec `
        -Tail $AccessLogTail

    $ok = $ok -and $fileOk
} else {
    Write-Host "AccessLogFileCheck: skipped"
}

$lokiQuery = Wait-LokiRequestIdQuery `
    -Endpoint $LokiUrl `
    -Expression $lokiExpression `
    -StartUtc $queryStartUtc `
    -WaitSec $LokiWaitSec `
    -IntervalSec $RetryIntervalSec `
    -Limit $LokiLimit `
    -RequestTimeoutSec $TimeoutSec `
    -QueryLabel ""

if (-not $lokiQuery.Matched) {
    Write-Host "LokiMatched: false"
    if ($lokiQuery.LastError) {
        Write-Host "LokiDiagnostic: $($lokiQuery.LastError)"
    } else {
        Write-Host "LokiDiagnostic: no entries matched the generated request ID before the retry deadline"
    }
    $ok = $false
} else {
    Write-Host "LokiMatched: true"
    $classificationOk = Test-LokiClassificationFields `
        -JsonEntries $lokiQuery.Result.JsonEntries `
        -ExpectedRouteFamily "gateway_probe" `
        -ExpectedStatusClass "2xx"
    $ok = $ok -and $classificationOk
}

$redactionLokiQuery = Wait-LokiRequestIdQuery `
    -Endpoint $LokiUrl `
    -Expression $redactionLokiExpression `
    -StartUtc $queryStartUtc `
    -WaitSec $LokiWaitSec `
    -IntervalSec $RetryIntervalSec `
    -Limit $LokiLimit `
    -RequestTimeoutSec $TimeoutSec `
    -QueryLabel "Redaction"

if (-not $redactionLokiQuery.Matched) {
    Write-Host "RedactionLokiMatched: false"
    if ($redactionLokiQuery.LastError) {
        Write-Host "RedactionLokiDiagnostic: $($redactionLokiQuery.LastError)"
    } else {
        Write-Host "RedactionLokiDiagnostic: no entries matched the generated redaction request ID before the retry deadline"
    }
    $ok = $false
} else {
    Write-Host "RedactionLokiMatched: true"
    $redactionOk = Test-LokiRedaction `
        -Lines $redactionLokiQuery.Result.Lines `
        -JsonEntries $redactionLokiQuery.Result.JsonEntries `
        -Secret $redactionSecret
    $ok = $ok -and $redactionOk
}

if (-not $ok) {
    exit 1
}

