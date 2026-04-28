@echo off
cd /d D:\MemoChat-Qml\server\VarifyServer
set MEMOCHAT_ENABLE_KAFKA=1
set MEMOCHAT_ENABLE_RABBITMQ=1
set MEMOCHAT_HEALTH_PORT=8082
start "VarifyServer" node server.js
