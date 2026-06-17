param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

Write-Host "=========================================="
Write-Host "  Reconfigure build-msquic"
Write-Host "=========================================="

# Keep the existing CMake cache by default. Deleting it forces vcpkg to
# re-evaluate the full manifest and is the main cause of repeated dependency work.
$cacheFile = "d:\MemoChat-Qml-Drogon\build-msquic\CMakeCache.txt"
if ($Force -and (Test-Path $cacheFile)) {
    Write-Host "[REMOVE] CMakeCache.txt"
    Remove-Item $cacheFile -Force
}

# Environment for correct Windows SDK
$env:LIB = "D:/VS2026stable/VC/Tools/MSVC/14.50.35717/lib/x64;C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/ucrt/x64;C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64"
$env:WINSDK_LIB_UM = "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64"
$env:WINSDK_LIB_UCRT = "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/ucrt/x64"
$env:VC_TOOLS_LIB = "D:/VS2026stable/VC/Tools/MSVC/14.50.35717/lib/x64"
$env:WINSDK_BIN = "C:/Program Files (x86)/Windows Kits/10/bin/10.0.28000.0/x64"
$env:PATH = "$env:WINSDK_BIN;D:/VS2026stable/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin;D:/VS2026stable/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64;$env:PATH"
$env:VCPKG_ROOT = "D:/vcpkg"
$env:VCPKG_DOWNLOADS = "D:/vcpkg/downloads"
$env:VCPKG_DEFAULT_BINARY_CACHE = "D:/vcpkg/bincache"
$env:VCPKG_BINARY_SOURCES = "clear;files,D:/vcpkg/bincache,readwrite"
$env:VCPKG_BUILD_TYPE = "release"

Write-Host "[ENV] LIB = $($env:LIB)"
Write-Host "[ENV] WINSDK_LIB_UM = $($env:WINSDK_LIB_UM)"

$cmakeBin = "C:\Program Files\CMake\bin\cmake.exe"
$cmakeArgs = @(
    "-S", "d:\MemoChat-Qml-Drogon",
    "-B", "d:\MemoChat-Qml-Drogon\build-msquic",
    "-G", "Ninja Multi-Config",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DBUILD_CLIENT=ON",
    "-DBUILD_OPS=ON",
    "-DBUILD_SERVER=OFF",
    "-DBUILD_TESTS=OFF",
    "-DCMAKE_MAKE_PROGRAM=D:/ninja-win/ninja.exe",
    "-DVCPKG_ROOT=D:/vcpkg",
    "-DVCPKG_DOWNLOADS=D:/vcpkg/downloads",
    "-DVCPKG_DEFAULT_BINARY_CACHE=D:/vcpkg/bincache",
    "-DVCPKG_BINARY_SOURCES=clear;files,D:/vcpkg/bincache,readwrite",
    "-DVCPKG_TARGET_TRIPLET=x64-windows",
    "-DVCPKG_HOST_TRIPLET=x64-windows",
    "-DVCPKG_INSTALLED_DIR=d:/MemoChat-Qml-Drogon/vcpkg_installed",
    "-DVCPKG_MANIFEST_FEATURES=server",
    "-DVCPKG_BUILD_TYPE=release",
    "-DCMAKE_CXX_COMPILER=D:/VS2026stable/VC/Tools/MSVC/14.50.35717/bin/Hostx64/x64/cl.exe"
)

Write-Host "[RUN] cmake configure..."
& $cmakeBin @cmakeArgs 2>&1 | Select-Object -Last 50

if ($LASTEXITCODE -ne 0) {
    Write-Host "[FAIL] CMake configure failed with exit code $LASTEXITCODE"
    exit 1
}

Write-Host "[OK] CMake configure succeeded"
