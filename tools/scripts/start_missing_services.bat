@echo off
set MEMOCHAT_ENABLE_KAFKA=1
set MEMOCHAT_ENABLE_RABBITMQ=1
set RUNTIME_DIR=D:\MemoChat-Qml\Memo_ops\runtime\services

echo Starting ChatServer-1...
start "ChatServer-1" cmd /k "cd /d %RUNTIME_DIR%\chatserver1 && set MEMOCHAT_ENABLE_KAFKA=1 && set MEMOCHAT_ENABLE_RABBITMQ=1 && ChatServer.exe --config config.ini"

echo Starting ChatServer-2...
start "ChatServer-2" cmd /k "cd /d %RUNTIME_DIR%\chatserver2 && set MEMOCHAT_ENABLE_KAFKA=1 && set MEMOCHAT_ENABLE_RABBITMQ=1 && ChatServer.exe --config config.ini"

echo All services started.
