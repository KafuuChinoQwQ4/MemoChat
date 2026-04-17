@echo off
REM ============================================================
REM  将 build\bin\Release 的可执行文件部署到 Memo_ops\runtime\services
REM  自动适配项目根目录（从脚本所在位置推算）
REM ============================================================
setlocal enabledelayedexpansion
cd /d "%~dp0..\.."
set "PROJECT_ROOT=%CD%"
set "BUILD_BIN=%PROJECT_ROOT%\build\bin\Release"
set "RUNTIME_DIR=%PROJECT_ROOT%\Memo_ops\runtime\services"

echo ============================================================
echo   Deploy C++ services to runtime directory
echo   PROJECT_ROOT: %PROJECT_ROOT%
echo   BUILD_BIN:    %BUILD_BIN%
echo   RUNTIME_DIR:  %RUNTIME_DIR%
echo ============================================================
echo.

REM ---- 清理旧的运行时目录（确保干净） ----
if exist "%RUNTIME_DIR%" (
    echo [CLEAN] Deleting old runtime directory...
    rmdir /s /q "%RUNTIME_DIR%"
)
echo.

REM ---- 创建各服务子目录 ----
echo [MKDIR] Creating service subdirectories...
mkdir "%RUNTIME_DIR%\GateServer"        2>nul
mkdir "%RUNTIME_DIR%\StatusServer"      2>nul
mkdir "%RUNTIME_DIR%\chatserver1"       2>nul
mkdir "%RUNTIME_DIR%\chatserver2"       2>nul
mkdir "%RUNTIME_DIR%\chatserver3"       2>nul
mkdir "%RUNTIME_DIR%\chatserver4"       2>nul
mkdir "%RUNTIME_DIR%\GateServerHttp1.1" 2>nul
mkdir "%RUNTIME_DIR%\GateServerHttp2"    2>nul
mkdir "%RUNTIME_DIR%\MemoChatQml"       2>nul
mkdir "%RUNTIME_DIR%\MemoOpsQml"        2>nul
echo.

REM ---- 拷贝各服务 exe ----
call :copyOne "%BUILD_BIN%\GateServer.exe"           "%RUNTIME_DIR%\GateServer\GateServer.exe"
call :copyOne "%BUILD_BIN%\StatusServer.exe"         "%RUNTIME_DIR%\StatusServer\StatusServer.exe"
call :copyOne "%BUILD_BIN%\ChatServer.exe"           "%RUNTIME_DIR%\chatserver1\ChatServer.exe"
call :copyOne "%BUILD_BIN%\ChatServer.exe"            "%RUNTIME_DIR%\chatserver2\ChatServer.exe"
call :copyOne "%BUILD_BIN%\ChatServer.exe"            "%RUNTIME_DIR%\chatserver3\ChatServer.exe"
call :copyOne "%BUILD_BIN%\ChatServer.exe"            "%RUNTIME_DIR%\chatserver4\ChatServer.exe"
call :copyOne "%BUILD_BIN%\GateServerHttp1.1.exe"    "%RUNTIME_DIR%\GateServerHttp1.1\GateServerHttp1.1.exe"
call :copyOne "%BUILD_BIN%\GateServerHttp1.1.exe"    "%RUNTIME_DIR%\GateServerHttp2\GateServerHttp1.1.exe"
call :copyOne "%BUILD_BIN%\GateServerDrogon.exe"     "%RUNTIME_DIR%\GateServer\GateServerDrogon.exe"
call :copyOne "%BUILD_BIN%\MemoChatQml.exe"        "%RUNTIME_DIR%\MemoChatQml\MemoChatQml.exe"
call :copyOne "%BUILD_BIN%\MemoOpsQml.exe"        "%RUNTIME_DIR%\MemoOpsQml\MemoOpsQml.exe"
echo.

REM ---- 拷贝 vcpkg 依赖 DLL ----
set "VCPKG_DIR=%PROJECT_ROOT%\vcpkg_installed\x64-windows\bin"
if exist "%VCPKG_DIR%" (
    echo [DLL] Copying vcpkg DLLs...
    for %%F in (
        "drogon.dll"
        "abseil_dll.dll"
        "aws-c-auth.dll"
        "aws-c-cal.dll"
        "aws-c-common.dll"
        "aws-c-compression.dll"
        "aws-c-event-stream.dll"
        "aws-c-http.dll"
        "aws-c-io.dll"
        "aws-c-mqtt.dll"
        "aws-c-s3.dll"
        "aws-c-sdkutils.dll"
        "aws-checksums.dll"
        "aws-cpp-sdk-core.dll"
        "aws-cpp-sdk-s3.dll"
        "aws-crt-cpp.dll"
        "boost_atomic-vc145-mt-x64-1_90.dll"
        "boost_chrono-vc145-mt-x64-1_90.dll"
        "boost_container-vc145-mt-x64-1_90.dll"
        "boost_context-vc145-mt-x64-1_90.dll"
        "boost_date_time-vc145-mt-x64-1_90.dll"
        "boost_filesystem-vc145-mt-x64-1_90.dll"
        "boost_program_options-vc145-mt-x64-1_90.dll"
        "boost_serialization-vc145-mt-x64-1_90.dll"
        "boost_thread-vc145-mt-x64-1_90.dll"
        "boost_wserialization-vc145-mt-x64-1_90.dll"
        "brotlicommon.dll"
        "brotlidec.dll"
        "brotlienc.dll"
        "bson-1.0.dll"
        "bsoncxx-v_noabi-rhi-md.dll"
        "cares.dll"
        "cppkafka.dll"
        "fmt.dll"
        "hiredis.dll"
        "jsoncpp.dll"
        "libcrypto-3-x64.dll"
        "libssl-3-x64.dll"
        "libpq.dll"
        "libprotobuf.dll"
        "libprotobuf-lite.dll"
        "lz4.dll"
        "mongoc-1.0.dll"
        "mongocxx-v_noabi-rhi-md.dll"
        "msquic.dll"
        "nghttp2.dll"
        "nghttp3.dll"
        "ngtcp2.dll"
        "pqxx.dll"
        "rabbitmq.4.dll"
        "rdkafka.dll"
        "rdkafka++.dll"
        "re2.dll"
        "spdlog.dll"
        "trantor.dll"
        "utf8proc.dll"
        "zlib1.dll"
    ) do (
        if exist "%VCPKG_DIR%\%%~nxF" copy /y "%VCPKG_DIR%\%%~nxF" "%RUNTIME_DIR%\GateServer\" >nul
        if exist "%VCPKG_DIR%\%%~nxF" copy /y "%VCPKG_DIR%\%%~nxF" "%RUNTIME_DIR%\StatusServer\" >nul
        if exist "%VCPKG_DIR%\%%~nxF" copy /y "%VCPKG_DIR%\%%~nxF" "%RUNTIME_DIR%\chatserver1\" >nul
        if exist "%VCPKG_DIR%\%%~nxF" copy /y "%VCPKG_DIR%\%%~nxF" "%RUNTIME_DIR%\chatserver2\" >nul
        if exist "%VCPKG_DIR%\%%~nxF" copy /y "%VCPKG_DIR%\%%~nxF" "%RUNTIME_DIR%\chatserver3\" >nul
        if exist "%VCPKG_DIR%\%%~nxF" copy /y "%VCPKG_DIR%\%%~nxF" "%RUNTIME_DIR%\chatserver4\" >nul
    )
    echo [DLL] Done
)
echo.

REM ---- 拷贝各服务 config.ini ----
set "SRC=%PROJECT_ROOT%\server"
call :copyOne "%SRC%\StatusServer\config.ini"        "%RUNTIME_DIR%\StatusServer\config.ini"
call :copyOne "%SRC%\GateServer\config.ini"           "%RUNTIME_DIR%\GateServer\config.ini"
call :copyOne "%SRC%\ChatServer\config.ini"           "%RUNTIME_DIR%\chatserver1\config.ini"
call :copyOne "%SRC%\ChatServer\config.ini"           "%RUNTIME_DIR%\chatserver2\config.ini"
call :copyOne "%SRC%\ChatServer\config.ini"           "%RUNTIME_DIR%\chatserver3\config.ini"
call :copyOne "%SRC%\ChatServer\config.ini"           "%RUNTIME_DIR%\chatserver4\config.ini"
call :copyOne "%SRC%\GateServerHttp1.1\config.ini"   "%RUNTIME_DIR%\GateServerHttp1.1\config.ini"
call :copyOne "%SRC%\GateServerHttp2\config.ini"     "%RUNTIME_DIR%\GateServerHttp2\config.ini"
call :copyOne "%BUILD_BIN%\config.ini"                "%RUNTIME_DIR%\MemoChatQml\config.ini"
call :copyOne "%BUILD_BIN%\memoops-qml.ini"           "%RUNTIME_DIR%\MemoOpsQml\memoops-qml.ini"
echo.

REM ---- 为每个 ChatServer 实例生成唯一配置 ----
REM chatserver2/3/4 的 config.ini 仍指向 chatserver1，导致 Snowflake ID 冲突。
REM 这里用 PowerShell 原地修改 [SelfServer] Name 和 [Snowflake] WorkerId。
call :patch_chat_config "%RUNTIME_DIR%\chatserver1\config.ini" "chatserver1" "1"
call :patch_chat_config "%RUNTIME_DIR%\chatserver2\config.ini" "chatserver2" "2"
call :patch_chat_config "%RUNTIME_DIR%\chatserver3\config.ini" "chatserver3" "3"
call :patch_chat_config "%RUNTIME_DIR%\chatserver4\config.ini" "chatserver4" "4"
echo.

REM ---- 验证 ----
echo [VERIFY] Checking deployment results...
set "ALL_OK=1"
call :check "%RUNTIME_DIR%\GateServer\GateServer.exe"
call :check "%RUNTIME_DIR%\StatusServer\StatusServer.exe"
call :check "%RUNTIME_DIR%\chatserver1\ChatServer.exe"
call :check "%RUNTIME_DIR%\chatserver2\ChatServer.exe"
call :check "%RUNTIME_DIR%\chatserver3\ChatServer.exe"
call :check "%RUNTIME_DIR%\chatserver4\ChatServer.exe"
call :check "%RUNTIME_DIR%\GateServer\config.ini"
call :check "%RUNTIME_DIR%\GateServer\config.ini"
call :check "%RUNTIME_DIR%\StatusServer\config.ini"
call :check "%RUNTIME_DIR%\chatserver1\config.ini"
call :check "%RUNTIME_DIR%\chatserver2\config.ini"
call :check "%RUNTIME_DIR%\chatserver3\config.ini"
call :check "%RUNTIME_DIR%\chatserver4\config.ini"
call :check "%RUNTIME_DIR%\MemoChatQml\MemoChatQml.exe"
call :check "%RUNTIME_DIR%\MemoChatQml\config.ini"
call :check "%RUNTIME_DIR%\MemoOpsQml\MemoOpsQml.exe"
call :check "%RUNTIME_DIR%\MemoOpsQml\memoops-qml.ini"
echo.
if "!ALL_OK!"=="1" (
    echo [SUCCESS] All services deployed to %RUNTIME_DIR%
) else (
    echo [FAIL] Some files missing, check above
)
echo.
echo Run start-all-services.bat to start all services.
pause
goto :eof

:patch_chat_config
REM %1 = config file path, %2 = SelfServer.Name, %3 = Snowflake.WorkerId
set "CONF=%~1"
set "NAME=%~2"
set "WORKER=%~3"
if exist "%CONF%" (
    powershell -NoProfile -Command ^
        "$conf='%CONF%'; $name='%NAME%'; $worker='%WORKER%'; " ^
        "$lines=[System.IO.File]::ReadAllLines($conf); " ^
        "$out=@(); $in_self=$false; $in_snow=$false; $self_name_added=$false; $snow_ids_added=$false; foreach($l in $lines) { " ^
        "if($l -cmatch '^\[([^\]]+)\]$') { " ^
        "$in_self=($matches[1] -eq 'SelfServer'); " ^
        "$in_snow=($matches[1] -eq 'Snowflake'); " ^
        "$self_name_added=$false; $snow_ids_added=$false; " ^
        "$out+=$l; " ^
        "if($in_self) { $out+='Name='+$name; $self_name_added=$true } " ^
        "elseif($in_snow) { $out+='DatacenterId=1'; $out+='WorkerId='+$worker; $snow_ids_added=$true } } " ^
        "elseif($in_self -and $l -cmatch '^Name=' -and $self_name_added) { } " ^
        "elseif($in_snow -and ($l -cmatch '^DatacenterId=' -or $l -cmatch '^WorkerId=') -and $snow_ids_added) { } " ^
        "else { $out+=$l } }; " ^
        "[System.IO.File]::WriteAllLines($conf,$out); " ^
        "Write-Host '[PATCH]'$conf' Name='$name' WorkerId='$worker"
)
exit /b 0

:copyOne
if exist "%~1" (
    copy /y "%~1" "%~2" >nul
    if !ERRORLEVEL! equ 0 (
        echo [OK] %~nx1
    ) else (
        echo [FAIL] %~nx1
        set "ALL_OK=0"
    )
) else (
    echo [WARN] Source not found: %~1
)
exit /b 0

:check
if exist "%~1" (
    echo [OK] %~nx1
) else (
    echo [X] Missing: %~nx1
    set "ALL_OK=0"
)
exit /b 0
