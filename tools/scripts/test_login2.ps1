param(
    [string]$BaseUrl = "http://127.0.0.1",
    [string]$Email = "testuser123@loadtest.local",
    [string]$Password = "123456",
    [string]$ClientVersion = "3.0.0"
)

& (Join-Path $PSScriptRoot "test_login.ps1") -BaseUrl $BaseUrl -Email $Email -Password $Password -ClientVersion $ClientVersion
exit $LASTEXITCODE
