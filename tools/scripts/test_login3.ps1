$body = '{"email":"testuser123@loadtest.local","passwd":"123456","client_ver":"3.0.0"}'
try {
    $r = Invoke-WebRequest -Uri "http://127.0.0.1:8080/user_login" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 10
    Write-Host "Status: $($r.StatusCode)"
    Write-Host "Body: $($r.Content)"
} catch {
    $ex = $_.Exception
    Write-Host "Exception Type: $($ex.GetType().Name)"
    if ($ex.Response) {
        Write-Host "Status Code: $($ex.Response.StatusCode)"
        $reader = [System.IO.StreamReader]::new($ex.Response.GetResponseStream())
        $respBody = $reader.ReadToEnd()
        $reader.Close()
        Write-Host "Response Body: $respBody"
    } else {
        Write-Host "No Response: $($ex.Message)"
    }
}
