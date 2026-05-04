@echo off
setlocal enabledelayedexpansion

set "REPO_ROOT=%~dp0..\.."
for %%I in ("%REPO_ROOT%") do set "REPO_ROOT=%%~fI"
set "PY_LOADTEST=%REPO_ROOT%\tools\loadtest\python-loadtest\py_loadtest.py"
set "CONFIG=%REPO_ROOT%\tools\loadtest\python-loadtest\config.json"
set "REPORT_DIR=%REPO_ROOT%\infra\Memo_ops\artifacts\reports\loadtest"

echo ================================================
echo MemoChat Python protocol benchmark
echo ================================================
echo.

if not exist "%REPORT_DIR%" mkdir "%REPORT_DIR%"
del /q "%REPORT_DIR%\*.json" 2>nul

echo [1/3] HTTP login (100 concurrency, 500 requests)
python "%PY_LOADTEST%" ^
    --scenario http ^
    --config "%CONFIG%" ^
    --total 500 ^
    --concurrency 100 ^
    --report-dir "%REPORT_DIR%"

echo.
echo [2/3] TCP login (50 concurrency, 500 requests)
python "%PY_LOADTEST%" ^
    --scenario tcp ^
    --config "%CONFIG%" ^
    --total 500 ^
    --concurrency 50 ^
    --report-dir "%REPORT_DIR%"

echo.
echo [3/3] AI/RAG smoke benchmark (10 concurrency, 50 requests)
python "%PY_LOADTEST%" ^
    --scenario ai-health ^
    --config "%CONFIG%" ^
    --total 50 ^
    --concurrency 10 ^
    --report-dir "%REPORT_DIR%"

echo.
echo ================================================
echo Benchmark reports: %REPORT_DIR%
echo ================================================
