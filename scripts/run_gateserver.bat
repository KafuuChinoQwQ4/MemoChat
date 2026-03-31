@echo off
cd /d D:\MemoChat-Qml-Drogon\Memo_ops\runtime\services\GateServer
set MEMOCHAT_ENABLE_KAFKA=1
set MEMOCHAT_ENABLE_RABBITMQ=1
start "GateServer" GateServer.exe --config config.ini
