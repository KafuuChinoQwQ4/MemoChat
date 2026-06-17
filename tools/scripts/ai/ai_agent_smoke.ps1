[CmdletBinding()]
param(
    [string]$GateUrl = "http://127.0.0.1:8080",
    [int]$Uid = 0,
    [string]$LoginEmail = "",
    [string]$LoginPassword = "",
    [int]$TimeoutSec = 120
)

$ErrorActionPreference = "Stop"

function Invoke-JsonPost {
    param(
        [string]$Url,
        [hashtable]$Body,
        [int]$Timeout = 30
    )
    return Invoke-RestMethod -Uri $Url -Method POST -ContentType "application/json" -Body ($Body | ConvertTo-Json -Depth 10 -Compress) -TimeoutSec $Timeout
}

function Invoke-JsonGet {
    param(
        [string]$Url,
        [int]$Timeout = 30
    )
    return Invoke-RestMethod -Uri $Url -Method GET -TimeoutSec $Timeout
}

function Invoke-JsonStreamPostText {
    param(
        [string]$Url,
        [string]$Body,
        [int]$Timeout = 120
    )
    Add-Type -AssemblyName System.Net.Http
    $client = [System.Net.Http.HttpClient]::new()
    try {
        $client.Timeout = [TimeSpan]::FromSeconds($Timeout)
        $request = [System.Net.Http.HttpRequestMessage]::new([System.Net.Http.HttpMethod]::Post, $Url)
        $request.Headers.Accept.ParseAdd("text/event-stream")
        $request.Content = [System.Net.Http.StringContent]::new($Body, [Text.Encoding]::UTF8, "application/json")
        $response = $client.SendAsync($request).GetAwaiter().GetResult()
        [void]$response.EnsureSuccessStatusCode()
        return $response.Content.ReadAsStringAsync().GetAwaiter().GetResult()
    } finally {
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

function Invoke-WithRetry {
    param(
        [scriptblock]$Action,
        [int]$Attempts = 3,
        [int]$DelaySec = 10,
        [string]$Label = "operation"
    )
    $lastError = $null
    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        try {
            return & $Action
        } catch {
            $lastError = $_
            if ($attempt -lt $Attempts) {
                Write-Host "[WARN] $Label attempt $attempt failed: $($_.Exception.Message)"
                Start-Sleep -Seconds $DelaySec
                continue
            }
        }
    }
    throw $lastError
}

function Resolve-Uid {
    param(
        [string]$Email,
        [string]$Password,
        [int]$FallbackUid
    )
    if ([string]::IsNullOrWhiteSpace($Email) -or [string]::IsNullOrWhiteSpace($Password)) {
        if ($FallbackUid -le 0) {
            return Get-Random -Minimum 910000 -Maximum 990000
        }
        return $FallbackUid
    }

    $login = Invoke-JsonPost -Url "$GateUrl/user_login" -Body @{
        email = $Email
        passwd = $Password
        client_ver = "3.0.0"
    } -Timeout 15
    Assert-True ($login.error -eq 0) "Login failed: $($login.message)"
    return [int]$login.uid
}

function Parse-SseEvents {
    param([object]$RawContent)
    if ($RawContent -is [byte[]]) {
        $RawText = [Text.Encoding]::UTF8.GetString($RawContent)
    } elseif ($RawContent -is [array]) {
        $RawText = ($RawContent -join "`n")
    } else {
        $RawText = [string]$RawContent
    }
    $events = @()
    foreach ($match in [regex]::Matches($RawText, "(?m)^data:\s*(.+)$")) {
        $payload = $match.Groups[1].Value.Trim()
        if ([string]::IsNullOrWhiteSpace($payload) -or $payload -eq "[DONE]") {
            continue
        }
        $events += ($payload | ConvertFrom-Json -ErrorAction Stop)
    }
    if ($events.Count -eq 0) {
        foreach ($match in [regex]::Matches($RawText, "data:\s*(\{.*?\})(?:\s*(?:`r?`n){2}|$)", [Text.RegularExpressions.RegexOptions]::Singleline)) {
            $payload = $match.Groups[1].Value.Trim()
            if (-not [string]::IsNullOrWhiteSpace($payload)) {
                $events += ($payload | ConvertFrom-Json -ErrorAction Stop)
            }
        }
    }
    return ,$events
}

$resolvedUid = Resolve-Uid -Email $LoginEmail -Password $LoginPassword -FallbackUid $Uid
Write-Host "[INFO] Using uid: $resolvedUid"

$models = Invoke-JsonGet -Url "$GateUrl/ai/model/list" -Timeout 15
Assert-True ($models.code -eq 0) "Model list failed"
Assert-True ($models.models.Count -gt 0) "Model list returned no models"
$modelType = [string]$models.default_model.model_type
$modelName = [string]$models.default_model.model_name
Assert-True (-not [string]::IsNullOrWhiteSpace($modelType)) "default_model.model_type missing"
Assert-True (-not [string]::IsNullOrWhiteSpace($modelName)) "default_model.model_name missing"
Write-Host "[INFO] Default model: $modelType / $modelName"

$chat = Invoke-WithRetry -Label "chat" -Attempts 3 -DelaySec 12 -Action {
    Invoke-JsonPost -Url "$GateUrl/ai/chat" -Body @{
        uid = $resolvedUid
        session_id = ""
        content = "Say OK in English."
        model_type = $modelType
        model_name = $modelName
        metadata = @{
            max_tokens = 8
            temperature = 0
            enable_thinking = $false
        }
    } -Timeout $TimeoutSec
}
Assert-True ($chat.code -eq 0) "Chat failed: $($chat.message)"
Assert-True (-not [string]::IsNullOrWhiteSpace([string]$chat.session_id)) "Chat did not return session_id"
Assert-True (-not [string]::IsNullOrWhiteSpace([string]$chat.content)) "Chat returned empty content"
$sessionId = [string]$chat.session_id
Write-Host "[INFO] Chat session: $sessionId"

$streamBody = @{
    uid = $resolvedUid
    session_id = $sessionId
    content = "Say OK in English."
    model_type = $modelType
    model_name = $modelName
    metadata = @{
        max_tokens = 8
        temperature = 0
        enable_thinking = $false
    }
} | ConvertTo-Json -Depth 10 -Compress
$streamRawContent = Invoke-WithRetry -Label "chat-stream" -Attempts 2 -DelaySec 8 -Action {
    Invoke-JsonStreamPostText -Url "$GateUrl/ai/chat/stream" -Body $streamBody -Timeout $TimeoutSec
}
$streamEvents = Parse-SseEvents -RawContent $streamRawContent
Assert-True ($streamEvents.Count -gt 0) "Stream returned no SSE events"
$finalStreamEvent = $streamEvents[-1]
Assert-True ([bool]$finalStreamEvent.is_final) "Final SSE event missing is_final=true"
Write-Host "[INFO] Stream events: $($streamEvents.Count)"

$kbToken = "ai-smoke-$([Guid]::NewGuid().ToString('N'))"
$kbText = "MemoChat AI smoke document. Token=$kbToken"
$kbUpload = Invoke-JsonPost -Url "$GateUrl/ai/kb/upload" -Body @{
    uid = $resolvedUid
    file_name = "ai_smoke.txt"
    file_type = "txt"
    content = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($kbText))
} -Timeout $TimeoutSec
Assert-True ($kbUpload.code -eq 0) "KB upload failed: $($kbUpload.message)"
Assert-True (-not [string]::IsNullOrWhiteSpace([string]$kbUpload.kb_id)) "KB upload did not return kb_id"
$kbId = [string]$kbUpload.kb_id
Write-Host "[INFO] Uploaded KB: $kbId"

$kbList = Invoke-JsonGet -Url "$GateUrl/ai/kb/list?uid=$resolvedUid" -Timeout 30
Assert-True ($kbList.code -eq 0) "KB list failed"
$kbItem = $kbList.knowledge_bases | Where-Object { $_.kb_id -eq $kbId } | Select-Object -First 1
Assert-True ($null -ne $kbItem) "Uploaded KB not found in KB list"

$kbSearch = Invoke-JsonPost -Url "$GateUrl/ai/kb/search" -Body @{
    uid = $resolvedUid
    query = $kbToken
    top_k = 3
} -Timeout $TimeoutSec
Assert-True ($kbSearch.code -eq 0) "KB search failed"
Assert-True ($kbSearch.chunks.Count -gt 0) "KB search returned no chunks"

$kbDelete = Invoke-JsonPost -Url "$GateUrl/ai/kb/delete" -Body @{
    uid = $resolvedUid
    kb_id = $kbId
} -Timeout $TimeoutSec
Assert-True ($kbDelete.code -eq 0) "KB delete failed: $($kbDelete.message)"

$kbListAfterDelete = Invoke-JsonGet -Url "$GateUrl/ai/kb/list?uid=$resolvedUid" -Timeout 30
$kbItemAfterDelete = $kbListAfterDelete.knowledge_bases | Where-Object { $_.kb_id -eq $kbId } | Select-Object -First 1
Assert-True ($null -eq $kbItemAfterDelete) "Deleted KB still present after delete"

Write-Host "[PASS] AI agent smoke succeeded"
