param(
    [string]$BaseUrl = "http://127.0.0.1:8096",
    [string]$CaseId = "",
    [string]$TraceId = "",
    [int]$Uid = 0,
    [switch]$RunAll,
    [switch]$List
)

$ErrorActionPreference = "Stop"

if ($List) {
    Invoke-RestMethod -Method Get -Uri "$BaseUrl/agent/evals" | ConvertTo-Json -Depth 20
    exit 0
}

$body = @{
    case_id = $CaseId
    trace_id = $TraceId
    uid = $Uid
    run_all = [bool]$RunAll
} | ConvertTo-Json -Depth 20

$result = Invoke-RestMethod -Method Post -Uri "$BaseUrl/agent/evals/run" -ContentType "application/json" -Body $body
$result | ConvertTo-Json -Depth 20

if (-not $result.passed) {
    exit 1
}
