set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_BUILD_TYPE release)

if(NOT DEFINED ENV{VCPKG_ROOT} OR "$ENV{VCPKG_ROOT}" STREQUAL "")
    message(FATAL_ERROR "x64-linux-memochat-release requires VCPKG_ROOT")
endif()

file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _memochat_vcpkg_root)
set(_memochat_prefix_map
    "-ffile-prefix-map=${_memochat_vcpkg_root}=/usr/src/vcpkg -fmacro-prefix-map=${_memochat_vcpkg_root}=/usr/src/vcpkg")

# Some ports only consume the generic flags while CMake ports also consume the
# configuration-specific variants. Set both so binary-cache ABI calculation and
# all supported port build systems use the same reproducible path policy.
set(VCPKG_C_FLAGS "${_memochat_prefix_map}")
set(VCPKG_CXX_FLAGS "${_memochat_prefix_map}")
set(VCPKG_C_FLAGS_RELEASE "${_memochat_prefix_map}")
set(VCPKG_CXX_FLAGS_RELEASE "${_memochat_prefix_map}")

unset(_memochat_prefix_map)
unset(_memochat_vcpkg_root)
