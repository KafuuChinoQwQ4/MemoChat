@echo off
setlocal enabledelayedexpansion

set "REPO_ROOT=%~dp0..\.."
for %%I in ("%REPO_ROOT%") do set "REPO_ROOT=%%~fI"
set "PY_LOADTEST=%REPO_ROOT%\tools\loadtest\python-loadtest\py_loadtest.py"
set "CONFIG=%REPO_ROOT%\tools\loadtest\python-loadtest\config.benchmark.json"
set "K6_HTTP=%REPO_ROOT%\tools\loadtest\k6\run-http-bench.ps1"
set "LAYERED_SUITE=%REPO_ROOT%\tools\loadtest\run-layered-suite.ps1"
set "REPORT_DIR=%REPO_ROOT%\infra\Memo_ops\artifacts\loadtest\runtime\reports"

echo ================================================
echo MemoChat layered protocol benchmark
echo ================================================
echo.

if not exist "%REPORT_DIR%" mkdir "%REPORT_DIR%"
del /q "%REPORT_DIR%\*.json" 2>nul

echo [1/4] HTTP health baseline via k6 (50 concurrency, 1000 requests)
powershell -NoProfile -ExecutionPolicy Bypass -File "%K6_HTTP%" ^
    -Scenario health ^
    -ConfigPath "%CONFIG%" ^
    -Total 1000 ^
    -Concurrency 50

echo.
echo [2/4] HTTP login via k6 (100 concurrency, 1000 requests)
powershell -NoProfile -ExecutionPolicy Bypass -File "%K6_HTTP%" ^
    -Scenario login ^
    -ConfigPath "%CONFIG%" ^
    -Total 1000 ^
    -Concurrency 100

echo.
echo [3/4] TCP full login via Python (50 concurrency, 500 requests)
python "%PY_LOADTEST%" ^
    --scenario tcp ^
    --config "%CONFIG%" ^
    --total 500 ^
    --concurrency 50 ^
    --report-dir "%REPORT_DIR%"

echo.
echo [4/4] HTTP login sweep via k6 orchestration
python "%PY_LOADTEST%" ^
    --scenario sweep ^
    --config "%CONFIG%" ^
    --total 200 ^
    --concurrency 20 ^
    --report-dir "%REPORT_DIR%"

echo.
echo [optional] AI/RAG smoke benchmark (10 concurrency, 50 requests)
python "%PY_LOADTEST%" ^
    --scenario ai-health ^
    --config "%CONFIG%" ^
    --total 50 ^
    --concurrency 10 ^
    --report-dir "%REPORT_DIR%"

echo.
echo [recommended] split app/dependency suite (HTTP login, TCP login, DB read/write)
powershell -NoProfile -ExecutionPolicy Bypass -File "%LAYERED_SUITE%" ^
    -ConfigPath "%CONFIG%" ^
    -Total 1000 ^
    -Concurrency 100 ^
    -DbTotal 1000 ^
    -DbConcurrency 100 ^
    -ReportDir "%REPORT_DIR%"

echo.
echo ================================================
echo Benchmark reports: %REPORT_DIR%
echo ================================================
