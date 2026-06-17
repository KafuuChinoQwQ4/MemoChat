param(
    [string]$BaseUrl = "http://127.0.0.1",
    [string]$AccountsCsv = "infra/Memo_ops/artifacts/loadtest/runtime/accounts/accounts.local.csv",
    [string]$Email = "",
    [string]$Password = "",
    [string]$ClientVersion = "3.0.0",
    [int]$TimeoutSec = 8
)

$ErrorActionPreference = "Stop"

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptRoot)

function ConvertTo-MemoChatXorPassword {
    param([Parameter(Mandatory = $true)][string]$Raw)

    $xorCode = $Raw.Length % 255
    $chars = foreach ($ch in $Raw.ToCharArray()) {
        [char](([int][char]$ch) -bxor $xorCode)
    }
    return -join $chars
}

function Resolve-ProjectPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-Path -Path $ProjectRoot -ChildPath $Path
}

if (-not $Email -or -not $Password) {
    $csvPath = Resolve-ProjectPath -Path $AccountsCsv
    if (-not (Test-Path -LiteralPath $csvPath)) {
        throw "Accounts CSV not found: $csvPath. Pass -Email and -Password, or seed load-test accounts first."
    }
    $account = Import-Csv -LiteralPath $csvPath | Select-Object -First 1
    if (-not $account) {
        throw "Accounts CSV is empty: $csvPath"
    }
    if (-not $Email) { $Email = [string]$account.email }
    if (-not $Password) { $Password = [string]$account.password }
}

$payload = @{
    email      = $Email
    passwd     = ConvertTo-MemoChatXorPassword -Raw $Password
    client_ver = $ClientVersion
} | ConvertTo-Json -Compress

$uri = "$($BaseUrl.TrimEnd('/'))/user_login"
Write-Host "=== MemoChat login smoke ==="
Write-Host "URL: $uri"
Write-Host "Email: $Email"

$response = Invoke-WebRequest -Uri $uri -Method POST -ContentType "application/json" -Body $payload -TimeoutSec $TimeoutSec -UseBasicParsing
Write-Host "Status: $($response.StatusCode)"
Write-Host "Body: $($response.Content)"

$json = $response.Content | ConvertFrom-Json
$ok = ([int]$json.error -eq 0) -and $json.uid -and $json.login_ticket -and $json.host -and $json.port
if (-not $ok) {
    Write-Host "Login smoke failed: expected error=0 plus uid/login_ticket/host/port."
    exit 1
}
