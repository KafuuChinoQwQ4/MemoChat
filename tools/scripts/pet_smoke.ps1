[CmdletBinding()]
param(
    [string]$GateUrl = "http://127.0.0.1:8080",
    [int]$Uid = 0,
    [int]$TimeoutSec = 60
)

$ErrorActionPreference = "Stop"

function Invoke-JsonPost {
    param(
        [string]$Url,
        [hashtable]$Body,
        [hashtable]$Headers = @{},
        [int]$Timeout = 30
    )
    return Invoke-RestMethod `
        -Uri $Url `
        -Method POST `
        -Headers $Headers `
        -ContentType "application/json" `
        -Body ($Body | ConvertTo-Json -Depth 10 -Compress) `
        -TimeoutSec $Timeout
}

function Invoke-JsonGet {
    param(
        [string]$Url,
        [hashtable]$Headers = @{},
        [int]$Timeout = 30
    )
    return Invoke-RestMethod -Uri $Url -Method GET -Headers $Headers -TimeoutSec $Timeout
}

function Invoke-StreamGetText {
    param(
        [string]$Url,
        [hashtable]$Headers = @{},
        [int]$Timeout = 60
    )
    Add-Type -AssemblyName System.Net.Http
    $client = [System.Net.Http.HttpClient]::new()
    $cts = [System.Threading.CancellationTokenSource]::new([TimeSpan]::FromSeconds($Timeout))
    try {
        $client.Timeout = [System.Threading.Timeout]::InfiniteTimeSpan
        $request = [System.Net.Http.HttpRequestMessage]::new([System.Net.Http.HttpMethod]::Get, $Url)
        $request.Headers.Accept.ParseAdd("text/event-stream")
        foreach ($key in $Headers.Keys) {
            $request.Headers.TryAddWithoutValidation($key, [string]$Headers[$key]) | Out-Null
        }
        $response = $client.SendAsync(
            $request,
            [System.Net.Http.HttpCompletionOption]::ResponseHeadersRead,
            $cts.Token
        ).GetAwaiter().GetResult()
        [void]$response.EnsureSuccessStatusCode()
        $stream = $response.Content.ReadAsStreamAsync().GetAwaiter().GetResult()
        $buffer = New-Object byte[] 1024
        $builder = [System.Text.StringBuilder]::new()
        while (-not $cts.IsCancellationRequested) {
            $read = $stream.ReadAsync($buffer, 0, $buffer.Length, $cts.Token).GetAwaiter().GetResult()
            if ($read -le 0) {
                break
            }
            [void]$builder.Append([Text.Encoding]::UTF8.GetString($buffer, 0, $read))
            $text = $builder.ToString()
            if ($text.Contains("`n`n") -or $text.Contains("`r`n`r`n")) {
                break
            }
        }
        return @{
            Body = $builder.ToString()
            Headers = $response.Headers
        }
    } finally {
        $cts.Dispose()
        $client.Dispose()
    }
}

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

function Parse-SseEvents {
    param([object]$RawContent)
    $rawText = [string]$RawContent
    $events = @()
    foreach ($match in [regex]::Matches($rawText, "(?m)^data:\s*(.+)$")) {
        $payload = $match.Groups[1].Value.Trim()
        if ([string]::IsNullOrWhiteSpace($payload) -or $payload -eq "[DONE]") {
            continue
        }
        $events += ($payload | ConvertFrom-Json -ErrorAction Stop)
    }
    return ,$events
}

function Get-HeaderValue {
    param(
        [object]$Headers,
        [string]$Name
    )
    if ($Headers -eq $null -or -not $Headers.Contains($Name)) {
        return ""
    }
    return [string]($Headers.GetValues($Name) | Select-Object -First 1)
}

$traceId = "pet-smoke-trace-$([Guid]::NewGuid().ToString('N'))"
$requestId = "pet-smoke-request-$([Guid]::NewGuid().ToString('N'))"
$headers = @{
    "X-Trace-Id" = $traceId
    "X-Request-Id" = $requestId
}

Write-Host "[INFO] GateUrl: $GateUrl"
Write-Host "[INFO] Trace: $traceId"

$sessionRsp = Invoke-JsonPost -Url "$GateUrl/ai/pet/sessions" -Headers $headers -Body @{
    uid = $Uid
    profile_id = "smoke"
    persona = "memo-pet"
    provider = "scripted"
} -Timeout $TimeoutSec
Assert-True ($sessionRsp.code -eq 0) "Pet session create failed"
Assert-True (-not [string]::IsNullOrWhiteSpace([string]$sessionRsp.session.session_id)) "Pet session_id missing"
$sessionId = [string]$sessionRsp.session.session_id
Write-Host "[INFO] Pet session: $sessionId"

$listRsp = Invoke-JsonGet -Url "$GateUrl/ai/pet/sessions?uid=$Uid" -Headers $headers -Timeout $TimeoutSec
Assert-True ($listRsp.code -eq 0) "Pet session list failed"
Assert-True (($listRsp.sessions | Where-Object { $_.session_id -eq $sessionId } | Select-Object -First 1) -ne $null) "Created pet session not listed"

$inputRsp = Invoke-JsonPost -Url "$GateUrl/ai/pet/sessions/$sessionId/input" -Headers $headers -Body @{
    uid = $Uid
    content = "你好，桌宠 smoke"
    model_type = "scripted"
    model_name = "deterministic"
} -Timeout $TimeoutSec
Assert-True ($inputRsp.code -eq 0) "Pet input failed"
Assert-True ($inputRsp.events.Count -ge 3) "Pet input returned too few events"
$inputLast = $inputRsp.events[-1]
Assert-True ($inputLast.type -eq "pet.control") "Pet input did not return pet.control events"
Assert-True ($inputLast.phase -eq "idle") "Pet input final phase was not idle"

$observationRsp = Invoke-JsonPost -Url "$GateUrl/ai/pet/sessions/$sessionId/observation" -Headers $headers -Body @{
    audio = @{
        vad = "speaking"
        rms = 0.4
    }
    vision = @{
        enabled = $true
        mode = "landmarks_only"
        face_present = $true
        expression = "smile"
    }
    privacy = @{
        raw_frame_sent = $false
        raw_audio_recorded = $false
    }
} -Timeout $TimeoutSec
Assert-True ($observationRsp.code -eq 0) "Pet observation failed"
Assert-True ($observationRsp.event.privacy.camera_used -eq $true) "Observation did not reflect camera usage"
Assert-True ($observationRsp.event.privacy.raw_frame_sent -eq $false) "Observation leaked raw frame flag"

$interruptRsp = Invoke-JsonPost -Url "$GateUrl/ai/pet/sessions/$sessionId/interrupt" -Headers $headers -Body @{} -Timeout $TimeoutSec
Assert-True ($interruptRsp.code -eq 0) "Pet interrupt failed"
Assert-True ($interruptRsp.event.phase -eq "interrupted") "Interrupt phase mismatch"

$stream = Invoke-StreamGetText -Url "$GateUrl/ai/pet/sessions/$sessionId/stream" -Headers $headers -Timeout $TimeoutSec
$streamEvents = Parse-SseEvents -RawContent $stream.Body
Assert-True ($streamEvents.Count -gt 0) "Pet stream returned no SSE events"
Assert-True ($streamEvents[0].type -eq "pet.control") "Pet stream first event was not pet.control"

$responseTrace = Get-HeaderValue -Headers $stream.Headers -Name "X-Trace-Id"
$responseRequest = Get-HeaderValue -Headers $stream.Headers -Name "X-Request-Id"
Assert-True ($responseTrace -eq $traceId) "Trace header did not round-trip"
Assert-True ($responseRequest -eq $requestId) "Request header did not round-trip"

Write-Host "[PASS] Pet smoke succeeded"
