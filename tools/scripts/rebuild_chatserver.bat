@echo off
set "LIB=D:\VS2026stable\VC\Tools\MSVC\14.50.35717\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\winrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64;%LIB%"
cd /D D:\MemoChat-Qml-Drogon
set "VCPKG_ROOT=D:/vcpkg"
set "VCPKG_DOWNLOADS=D:/vcpkg/downloads"
set "VCPKG_DEFAULT_BINARY_CACHE=D:/vcpkg/bincache"
set "VCPKG_BINARY_SOURCES=clear;files,D:/vcpkg/bincache,readwrite"
cmake --preset msvc2022-full
if errorlevel 1 exit /b %errorlevel%
cmake --build --preset msvc2022-full --target ChatServer
