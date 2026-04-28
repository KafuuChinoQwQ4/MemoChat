$ErrorActionPreference = "Stop"

Write-Host "[INFO] This script no longer edits CMakeCache.txt directly."
Write-Host "[INFO] Use the stable preset build instead:"
Write-Host "       cmake --preset msvc2022-full"
Write-Host "       cmake --build --preset msvc2022-full"
Write-Host ""
Write-Host "[INFO] The preset pins vcpkg to D:/vcpkg, D:/vcpkg/bincache, x64-windows, and release-only packages."
