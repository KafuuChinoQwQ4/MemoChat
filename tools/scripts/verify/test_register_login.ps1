param(
    [string]$BaseUrl = "http://127.0.0.1",
    [string]$Email = "",
    [string]$User = "fullstack_smoke",
    [string]$Password = "FullStack123!",
    [string]$ClientVersion = "3.0.0",
    [int]$TimeoutSec = 10
)

$ErrorActionPreference = "Stop"
$DockerCli = Join-Path $PSScriptRoot "docker\arch-docker.ps1"

if (-not $Email) {
    $stamp = Get-Date -Format "yyyyMMddHHmmssfff"
    $suffix = ([guid]::NewGuid().ToString("N")).Substring(0, 8)
    $Email = "fullstack_${stamp}_${suffix}@loadtest.local"
    if ($User -eq "fullstack_smoke") {
        $User = "fullstack_$suffix"
    }
}

function ConvertTo-MemoChatXorPassword {
    param([Parameter(Mandatory = $true)][string]$Raw)

    $xorCode = $Raw.Length % 255
    $chars = foreach ($ch in $Raw.ToCharArray()) {
        [char](([int][char]$ch) -bxor $xorCode)
    }
    return -join $chars
}

function Invoke-JsonPost {
    param(
        [Parameter(Mandatory = $true)][string]$Uri,
        [Parameter(Mandatory = $true)][hashtable]$Body
    )

    $jsonBody = $Body | ConvertTo-Json -Compress
    $response = Invoke-WebRequest -Uri $Uri -Method POST -ContentType "application/json" -Body $jsonBody -TimeoutSec $TimeoutSec -UseBasicParsing
    Write-Host "Status: $($response.StatusCode)"
    Write-Host "Body: $($response.Content)"
    return ($response.Content | ConvertFrom-Json)
}

Write-Host "=== MemoChat register + login smoke ==="
Write-Host "BaseUrl: $BaseUrl"
Write-Host "Email: $Email"

Write-Host "=== Step 1: Get verify code ==="
$getCode = Invoke-JsonPost -Uri "$($BaseUrl.TrimEnd('/'))/get_varifycode" -Body @{ email = $Email }
if ([int]$getCode.error -ne 0) {
    Write-Host "Get verify code failed."
    exit 1
}

Write-Host "=== Step 2: Poll Redis for code ==="
$code = $null
for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Milliseconds 300
    $dockerArgs = @("exec", "-e", "REDISCLI_AUTH=123456", "memochat-redis", "redis-cli", "GET", "code_$Email")
    $value = & $DockerCli @dockerArgs 2>$null
    if ($LASTEXITCODE -eq 0 -and $value -and $value -ne "(nil)") {
        $code = [string]$value
        break
    }
}
if (-not $code) {
    Write-Host "Verify code not found in Redis for $Email."
    exit 1
}
Write-Host "Verify code: $code"

$encodedPassword = ConvertTo-MemoChatXorPassword -Raw $Password

Write-Host "=== Step 3: Register user ==="
$register = Invoke-JsonPost -Uri "$($BaseUrl.TrimEnd('/'))/user_register" -Body @{
    email      = $Email
    user       = $User
    passwd     = $encodedPassword
    confirm    = $encodedPassword
    icon       = ""
    varifycode = $code
    sex        = 0
}
if ([int]$register.error -ne 0) {
    Write-Host "Register failed."
    exit 1
}

Write-Host "=== Step 4: Login ==="
$login = Invoke-JsonPost -Uri "$($BaseUrl.TrimEnd('/'))/user_login" -Body @{
    email      = $Email
    passwd     = $encodedPassword
    client_ver = $ClientVersion
}
$ok = ([int]$login.error -eq 0) -and $login.uid -and $login.login_ticket -and $login.host -and $login.port
if (-not $ok) {
    Write-Host "Login after register failed."
    exit 1
}
