# Get verify code and read from Redis
$ErrorActionPreference = "Continue"

$email = "testuser123@loadtest.local"

# Step 1: Trigger verify code generation
Write-Host "Triggering verify code for $email..."
try {
    $body = '{"email":"' + $email + '"}'
    $r = Invoke-WebRequest -Uri "http://127.0.0.1:8080/get_varifycode" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 5
    Write-Host "Response: $($r.Content)"
} catch {
    Write-Host "Error: $($_.Exception.Message)"
}

# Step 2: Poll Redis for the code
Write-Host "Polling Redis..."
$dockerCli = Join-Path $PSScriptRoot "docker\arch-docker.ps1"
for ($i = 0; $i -lt 10; $i++) {
    Start-Sleep -Milliseconds 300
    $result = & $dockerCli exec -e REDISCLI_AUTH=123456 memochat-redis redis-cli GET "code_$email" 2>$null
    if ($LASTEXITCODE -eq 0 -and $result -and $result -ne "(nil)") {
        $code = [string]$result
        Write-Host "FOUND CODE: $code"
        break
    }
}
