$ErrorActionPreference = "Continue"
$email = "testuser123@loadtest.local"

Write-Host "=== Step 1: Get verify code ==="
try {
    $body = @{
        email = $email
    } | ConvertTo-Json -Compress
    Write-Host "Body: $body"
    $r = Invoke-WebRequest -Uri "http://127.0.0.1:8080/get_varifycode" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 5
    Write-Host "Status: $($r.StatusCode)"
    Write-Host "Response: $($r.Content)"
} catch {
    Write-Host "Error: $($_.Exception.Message)"
}

Write-Host "`n=== Step 2: Poll Redis for code ==="
$env:NODE_PATH = "D:\MemoChat-Qml-Drogon\server\VarifyServer\node_modules"
$node = "D:\Node.js\node.exe"
$found = $false
for ($i = 0; $i -lt 10; $i++) {
    Start-Sleep -Milliseconds 300
    $result = & $node "D:\MemoChat-Qml-Drogon\scripts\read_verify.js" $email 2>$null
    if ($result -match "Verify code: (\S+)") {
        $code = $matches[1]
        Write-Host "Found code: $code"
        $found = $true

        Write-Host "`n=== Step 3: Register user ==="
        $regBody = @{
            email     = $email
            user      = "testuser"
            passwd    = "123456"
            confirm   = "123456"
            icon      = ""
            varifycode = $code
            sex       = 0
        } | ConvertTo-Json -Compress
        Write-Host "RegBody: $regBody"
        try {
            $r2 = Invoke-WebRequest -Uri "http://127.0.0.1:8080/user_register" -Method POST -ContentType "application/json" -Body $regBody -TimeoutSec 5
            Write-Host "Reg Status: $($r2.StatusCode)"
            Write-Host "Reg Response: $($r2.Content)"
        } catch {
            Write-Host "Reg Error: $($_.Exception.Message)"
        }

        Write-Host "`n=== Step 4: Login ==="
        $loginBody = @{
            email      = $email
            passwd     = "123456"
            client_ver = "3.0.0"
        } | ConvertTo-Json -Compress
        try {
            $r3 = Invoke-WebRequest -Uri "http://127.0.0.1:8080/user_login" -Method POST -ContentType "application/json" -Body $loginBody -TimeoutSec 5
            Write-Host "Login Status: $($r3.StatusCode)"
            Write-Host "Login Response: $($r3.Content)"
        } catch {
            Write-Host "Login Error: $($_.Exception.Message)"
        }

        break
    }
}
if (-not $found) {
    Write-Host "No verify code found after 10 attempts"
}
