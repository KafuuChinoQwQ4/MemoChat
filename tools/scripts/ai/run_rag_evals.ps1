param(
    [string]$BaseUrl = "http://127.0.0.1:8096",
    [string]$CaseId = "",
    [int]$Uid = 0,
    [switch]$RunAll,
    [string]$MetadataFiltersJson = "{}"
)

$ErrorActionPreference = "Stop"

try {
    $metadataFilters = $MetadataFiltersJson | ConvertFrom-Json
} catch {
    Write-Error "MetadataFiltersJson must be a JSON object: $($_.Exception.Message)"
    exit 2
}

$payload = @{
    case_id = $CaseId
    uid = $Uid
    run_all = [bool]$RunAll
    metadata_filters = $metadataFilters
}

$json = $payload | ConvertTo-Json -Depth 16
$url = "$($BaseUrl.TrimEnd('/'))/kb/evals/run"

Write-Host "[RAG EVAL] POST $url"
$response = Invoke-RestMethod -Uri $url -Method Post -ContentType "application/json" -Body $json
$response | ConvertTo-Json -Depth 32

if (-not $response.passed) {
    exit 1
}
