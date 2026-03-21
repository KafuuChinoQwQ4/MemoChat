@echo off
cd /d "D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer"
"D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer\GateServer.exe" --config="D:\MemoChat-Qml-Drogon\server\GateServer\config.ini"
echo Exit code: %ERRORLEVEL%
