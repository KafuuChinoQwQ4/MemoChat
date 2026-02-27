@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

set "PROTO_FILE=%SCRIPT_DIR%\message.proto"
set "PROTOC_PATH=%SCRIPT_DIR%\..\..\vcpkg_installed\x64-windows\tools\protobuf\protoc.exe"
set "GRPC_PLUGIN_PATH=%SCRIPT_DIR%\..\..\vcpkg_installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe"

if not exist "%PROTO_FILE%" (
  echo [ERROR] proto file not found: %PROTO_FILE%
  exit /b 1
)

if not exist "%PROTOC_PATH%" (
  for %%I in (protoc.exe) do if not "%%~$PATH:I"=="" set "PROTOC_PATH=%%~$PATH:I"
)

if not exist "%GRPC_PLUGIN_PATH%" (
  for %%I in (grpc_cpp_plugin.exe) do if not "%%~$PATH:I"=="" set "GRPC_PLUGIN_PATH=%%~$PATH:I"
)

if not exist "%PROTOC_PATH%" (
  echo [ERROR] protoc not found in vcpkg_installed or PATH.
  exit /b 1
)

if not exist "%GRPC_PLUGIN_PATH%" (
  echo [ERROR] grpc_cpp_plugin not found in vcpkg_installed or PATH.
  exit /b 1
)

echo [INFO] Generating gRPC code...
"%PROTOC_PATH%" -I="%SCRIPT_DIR%" --grpc_out="%SCRIPT_DIR%" --plugin=protoc-gen-grpc="%GRPC_PLUGIN_PATH%" "%PROTO_FILE%"
if errorlevel 1 exit /b 1

echo [INFO] Generating C++ code...
"%PROTOC_PATH%" -I="%SCRIPT_DIR%" --cpp_out="%SCRIPT_DIR%" "%PROTO_FILE%"
if errorlevel 1 exit /b 1

echo [DONE] Proto code generated.
exit /b 0
