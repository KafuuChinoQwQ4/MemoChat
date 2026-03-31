# Test full registration + login flow
$ErrorActionPreference = "Continue"

# Step 1: Get verify code
Write-Host "=== Step 1: Get verify code ==="
try {
    $body = '{"email":"test@test.com"}'
    $r = Invoke-WebRequest -Uri "http://127.0.0.1:8080/get_varifycode" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 5
    Write-Host "Status: $($r.StatusCode)"
    Write-Host "Response: $($r.Content)"
} catch {
    Write-Host "Error: $($_.Exception.Message)"
}

# Step 2: Register user
Write-Host "`n=== Step 2: Register user ==="
try {
    $body = '{"email":"test@test.com","user":"testuser","passwd":"123456","confirm":"123456","icon":"","varifycode":"123456","sex":0}'
    $r = Invoke-WebRequest -Uri "http://127.0.0.1:8080/user_register" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 5
    Write-Host "Status: $($r.StatusCode)"
    Write-Host "Response: $($r.Content)"
} catch {
    Write-Host "Error: $($_.Exception.Message)"
}

# Step 3: Login
Write-Host "`n=== Step 3: Login ==="
try {
    $body = '{"email":"test@test.com","passwd":"123456","client_ver":"3.0.0"}'
    $r = Invoke-WebRequest -Uri "http://127.0.0.1:8080/user_login" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 5
    Write-Host "Status: $($r.StatusCode)"
    Write-Host "Response: $($r.Content)"
} catch {
    Write-Host "Error: $($_.Exception.Message)"
}
