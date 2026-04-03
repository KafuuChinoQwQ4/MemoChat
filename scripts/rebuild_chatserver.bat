@echo off
set "LIB=D:\VS2026stable\VC\Tools\MSVC\14.50.35717\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\winrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64;%LIB%"
cd /D D:\MemoChat-Qml\build-vcpkg-server
ninja -j4 -f build-Release.ninja ChatServer.exe
