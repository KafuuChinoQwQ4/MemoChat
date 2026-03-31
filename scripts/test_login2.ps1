$body = '{"email":"testuser123@loadtest.local","passwd":"123456","client_ver":"3.0.0"}'
try {
    $r = Invoke-WebRequest -Uri "http://127.0.0.1:8080/user_login" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 5
    Write-Host "Status: $($r.StatusCode)"
    Write-Host "Body: $($r.Content)"
} catch {
    Write-Host "Error: $($_.Exception.Message)"
    Write-Host "Status Code: $($_.Exception.Response.StatusCode)"
}
