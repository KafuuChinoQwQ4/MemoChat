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
for ($i = 0; $i -lt 10; $i++) {
    Start-Sleep -Milliseconds 300
    $result = cmd /c "set NODE_PATH=D:\MemoChat-Qml-Drogon\server\VarifyServer\node_modules && node D:\MemoChat-Qml-Drogon\scripts\read_verify.js $email 2>nul"
    if ($result -match "Verify code: (\w+)") {
        $code = $matches[1]
        Write-Host "FOUND CODE: $code"
        break
    }
}
