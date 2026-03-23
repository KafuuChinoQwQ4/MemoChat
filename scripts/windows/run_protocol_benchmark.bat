@echo off
setlocal enabledelayedexpansion

set "REPORT_DIR=D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\loadtest\runtime\reports"
set "LOG_DIR=D:\MemoChat-Qml-Drogon\Memo_ops\artifacts\loadtest\runtime\logs"
set "GATE_URL=http://127.0.0.1:8080"

echo ================================================
echo MemoChat 协议性能对比压测
echo ================================================
echo.

:: 创建报告目录
if not exist "%REPORT_DIR%" mkdir "%REPORT_DIR%"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

:: 清理旧报告
del /q "%REPORT_DIR%\*.json" 2>nul
del /q "%LOG_DIR%\*.log" 2>nul

echo [1/3] 测试 HTTP/1.1 登录 (100并发, 500总请求)
echo.

:: HTTP/1.1 登录测试
"D:\MemoChat-Qml-Drogon\local-loadtest-cpp\build\bin\Release\memochat_loadtest.exe" ^
    --scenario http ^
    --config "D:\MemoChat-Qml-Drogon\local-loadtest-cpp\config.json" ^
    --gate-url "%GATE_URL%" ^
    --total 500 ^
    --concurrency 100 ^
    --timeout 5 ^
    --output "%REPORT_DIR%\http_login.json" 2>&1

echo.
echo [2/3] 测试 TCP 长连接登录 (50并发, 500总请求)
echo.

:: TCP 长连接测试
"D:\MemoChat-Qml-Drogon\local-loadtest-cpp\build\bin\Release\memochat_loadtest.exe" ^
    --scenario tcp ^
    --config "D:\MemoChat-Qml-Drogon\local-loadtest-cpp\config.json" ^
    --gate-url "%GATE_URL%" ^
    --total 500 ^
    --concurrency 50 ^
    --timeout 5 ^
    --output "%REPORT_DIR%\tcp_login.json" 2>&1

echo.
echo [3/3] 测试 QUIC 长连接登录 (50并发, 500总请求)
echo.

:: QUIC 测试
"D:\MemoChat-Qml-Drogon\local-loadtest-cpp\build\bin\Release\memochat_loadtest.exe" ^
    --scenario quic ^
    --config "D:\MemoChat-Qml-Drogon\local-loadtest-cpp\config.json" ^
    --gate-url "%GATE_URL%" ^
    --total 500 ^
    --concurrency 50 ^
    --timeout 5 ^
    --output "%REPORT_DIR%\quic_login.json" 2>&1

echo.
echo ================================================
echo 压测完成，报告在: %REPORT_DIR%
echo ================================================
