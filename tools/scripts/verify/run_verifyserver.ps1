$ErrorActionPreference = "Stop"

# VarifyServer is now a C++ service managed by the status scripts.
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $scriptRoot "status\deploy_services.bat")
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& (Join-Path $scriptRoot "status\start-all-services.bat")
exit $LASTEXITCODE
