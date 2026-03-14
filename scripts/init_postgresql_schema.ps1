param(
    [string]$OpsConfig = "Memo_ops/config/opsserver.yaml",
    [string]$BusinessSql = "migrations/postgresql/business/001_baseline.sql",
    [string]$MemoOpsSql = "migrations/postgresql/memo_ops/001_baseline.sql",
    [string]$AppSchema = "memo",
    [string]$OpsSchema = "memo_ops",
    [string]$WslDistro = "Ubuntu-24.04"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$opsConfigPath = Resolve-Path (Join-Path $repoRoot $OpsConfig)
$businessSqlPath = Resolve-Path (Join-Path $repoRoot $BusinessSql)
$memoOpsSqlPath = Resolve-Path (Join-Path $repoRoot $MemoOpsSql)

$opsConfigJson = @'
import json
import sys
from pathlib import Path

import yaml

payload = yaml.safe_load(Path(sys.argv[1]).read_text(encoding="utf-8")) or {}
pg = payload.get("postgresql")
if not isinstance(pg, dict):
    raise SystemExit("postgresql config missing")
print(json.dumps(pg))
'@ | python - $opsConfigPath

if (-not $opsConfigJson) {
    throw "failed to load postgresql config from $opsConfigPath"
}

$pg = $opsConfigJson | ConvertFrom-Json
$pgHost = [string]$pg.host
$pgPort = [string]$pg.port
$pgUser = [string]$pg.user
$password = [string]$pg.password
$pgDatabase = [string]$pg.database
$pgSslMode = if ($pg.sslmode) { [string]$pg.sslmode } else { "disable" }

function Invoke-WslPsql {
    param(
        [string]$SqlText,
        [string]$DatabaseName
    )

    $connInfo = "host=$pgHost port=$pgPort user=$pgUser dbname=$DatabaseName sslmode=$pgSslMode"
    $bashCommand = "export PGPASSWORD='$password'; psql '$connInfo' -v ON_ERROR_STOP=1"
    $SqlText | wsl.exe -d $WslDistro -- bash -lc $bashCommand
    if ($LASTEXITCODE -ne 0) {
        throw "psql failed for database $DatabaseName"
    }
}

$createDbSql = @"
SELECT 'CREATE DATABASE ""$pgDatabase""'
WHERE NOT EXISTS (SELECT 1 FROM pg_database WHERE datname = '$pgDatabase')\gexec
"@

Invoke-WslPsql -SqlText $createDbSql -DatabaseName "postgres"

$businessSqlText = (Get-Content $businessSqlPath -Raw).Replace("__APP_SCHEMA__", $AppSchema)
$memoOpsSqlText = (Get-Content $memoOpsSqlPath -Raw).Replace("__OPS_SCHEMA__", $OpsSchema)

Invoke-WslPsql -SqlText $businessSqlText -DatabaseName $pgDatabase
Invoke-WslPsql -SqlText $memoOpsSqlText -DatabaseName $pgDatabase

Write-Host "PostgreSQL schema initialized for app schema '$AppSchema' and ops schema '$OpsSchema'."
