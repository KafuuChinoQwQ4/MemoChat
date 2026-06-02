@echo off
REM Legacy wrapper. VarifyServer is now deployed and started as a C++ service.
setlocal
call "%~dp0status\deploy_services.bat"
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
call "%~dp0status\start-all-services.bat"
exit /b %ERRORLEVEL%
