@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM =====================================================
REM MemoChat: One-Command Build & Start Everything
REM
REM What it does (in order):
REM   1. CMake configure + build: server, client (in build/)
REM   2. CMake configure + build: MemoOps         (in build-memo-ops/)
REM   3. Stop any running services
REM   4. Start backend: VarifyServer, StatusServer,
REM      ChatServer(x4), GateServer, GateServerDrogon
REM   5. Start MemoOps: ops_server, ops_collector,
REM      MemoOpsQml
REM   6. Start MemoChat client (if --with-client)
REM
REM Usage:
REM   scripts\status\start_all_services.bat           <- build + all services
REM   scripts\status\start_all_services.bat --build   <- just build
REM   scripts\status\start_all_services.bat --start   <- just start (skip build)
REM   scripts\status\start_all_services.bat --with-client
REM   scripts\status\start_all_services.bat --no-client
REM   scripts\status\start_all_services.bat --help
REM =====================================================

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"

set "DO_BUILD=1"
set "DO_START=1"
set "WITH_CLIENT=1"
set "BUILD_PRESET=msvc2022-all-msquic"
set "BUILD_TARGET=--target build-all"

REM Detect CPU count for parallel builds
set "PARALLEL_COUNT=4"
where nproc >nul 2>nul
if not errorlevel 1 (
    for /f "delims=" %%C in ('nproc') do set "PARALLEL_COUNT=%%C"
) else (
    for /f "delims=" %%C in ('powershell -Command "[Environment]::ProcessorCount"') do set "PARALLEL_COUNT=%%C"
    if not defined PARALLEL_COUNT set "PARALLEL_COUNT=4"
)

for %%A in (%*) do (
    if /I "%%~A"=="--help"       set "HELP=1"
    if /I "%%~A"=="--build"      set "DO_START=0"
    if /I "%%~A"=="--start"      set "DO_BUILD=0"
    if /I "%%~A"=="--no-client"  set "WITH_CLIENT=0"
    if /I "%%~A"=="--with-client" set "WITH_CLIENT=1"
    if /I "%%~A"=="--client-only" set "BUILD_TARGET=--target MemoChatQml"
)

if defined HELP (
    echo.
    echo Usage: start_all_services.bat [options]
    echo.
    echo Options:
    echo   --build        Build all components only, do not start services
    echo   --start        Start services only, skip building
    echo   --with-client  Also launch MemoChat QML client ^(default^)
    echo   --no-client    Skip MemoChat QML client launch
    echo   --client-only  Build only the MemoChat client
    echo   --help         Show this help message
    echo.
    echo Examples:
    echo   start_all_services.bat                # Build + start everything
    echo   start_all_services.bat --build        # Just build
    echo   start_all_services.bat --start        # Start with existing binaries
    echo   start_all_services.bat --no-client    # Skip client launch
    echo.
    exit /b 0
)

echo =====================================================
echo MemoChat: Build ^& Start Everything
echo =====================================================
echo ROOT:          %ROOT%
echo BUILD:         %DO_BUILD%
echo START:         %DO_START%
echo CLIENT:        %WITH_CLIENT%
echo BUILD PRESET:  %BUILD_PRESET%
echo PARALLEL:      %PARALLEL_COUNT%
echo =====================================================
echo.

REM =====================================================
REM STEP 1: BUILD
REM =====================================================
if "%DO_BUILD%"=="1" (
    echo [STEP 1/4] Configuring and building MemoChat ^(server + client^)...
    echo [INFO] This may take several minutes on first run...
    echo.

    REM Configure via preset
    cmake --preset %BUILD_PRESET%
    if errorlevel 1 (
        echo [ERROR] CMake configure preset failed: %BUILD_PRESET%
        echo [HINT] Check: cmake --preset list
        exit /b 1
    )

    REM Build server + client
    echo [BUILD] Building server + client targets...
    cmake --build --preset msvc2022-msquic-release --target build-all --parallel %PARALLEL_COUNT%
    if errorlevel 1 (
        echo [ERROR] MemoChat build failed.
        exit /b 1
    )
    echo [BUILD] MemoChat ^(server + client^) built successfully.

    echo.
    echo [STEP 2/4] Configuring and building MemoOps...
    echo.

    pushd "%ROOT%\Memo_ops" >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] Failed to enter Memo_ops directory.
        exit /b 1
    )

    cmake --preset msvc2022-memo-ops
    if errorlevel 1 (
        popd
        echo [ERROR] MemoOps configure preset failed.
        exit /b 1
    )

    echo [BUILD] Building MemoOps QML client...
    cmake --build --preset msvc2022-memo-ops-release --parallel %PARALLEL_COUNT%
    if errorlevel 1 (
        popd
        echo [WARN] MemoOps build had errors. Continuing anyway...
    ) else (
        echo [BUILD] MemoOps built successfully.
    )
    popd
) else (
    echo [SKIP] Build step skipped ^(--start mode^)
    echo.
    echo [STEP 2/4] Skipped ^(build^)
)

REM =====================================================
REM STEP 3: STOP OLD SERVICES
REM =====================================================
echo.
echo [STEP 3/4] Stopping any running services...
call "%ROOT%\scripts\windows\stop_test_services.bat" >nul 2>nul
echo [INFO] Old services stopped.

REM =====================================================
REM STEP 4: START ALL SERVICES
REM =====================================================
echo.
echo [STEP 4/4] Starting all services...
echo.

REM Start backend + MemoOps
call "%ROOT%\scripts\windows\start_test_services.bat"
if errorlevel 1 (
    echo [ERROR] Backend services failed to start.
    echo [HINT] Check Memo_ops\artifacts\logs\manual-start\ for error logs.
    exit /b 1
)

echo.
echo =====================================================
echo All services started successfully!
echo =====================================================
echo.
echo Running services:
echo   - GateServer      : http://127.0.0.1:8080
echo   - GateServerDrogon: https://127.0.0.1:8443 ^(HTTP/2^)
echo   - StatusServer    : gRPC 127.0.0.1:50052
echo   - VarifyServer    : gRPC 127.0.0.1:50051
echo   - ChatServer(x4) : TCP 8090/8091/8092/8093
echo   - MemoOps Server  : Python ^(ops_server + ops_collector^)
echo   - MemoOpsQml      : Monitor UI
if "%WITH_CLIENT%"=="1" (
    echo   - MemoChatQml   : Chat client
)
echo.
echo Logs: Memo_ops\artifacts\logs\manual-start\
echo.
echo To stop:  scripts\windows\stop_test_services.bat
echo To rebuild: scripts\status\start_all_services.bat --build
echo =====================================================

exit /b 0
