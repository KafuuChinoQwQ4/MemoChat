param(
    [string]$BaseUrl = "http://127.0.0.1"
)

& (Join-Path $PSScriptRoot "test_register_login.ps1") -BaseUrl $BaseUrl
exit $LASTEXITCODE
