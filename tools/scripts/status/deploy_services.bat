@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0deploy_services.ps1"
exit /b %ERRORLEVEL%
