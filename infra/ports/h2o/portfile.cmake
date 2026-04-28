# vcpkg port for h2o
# Homepage: https://h2o.examp1e.net/

set(H2O_VERSION "2.2.6")
set(H2O_SOURCE_URL "https://github.com/h2o/h2o/archive/refs/tags/v${H2O_VERSION}.tar.gz")

vcpkg_download_distfile(DIST_ARCHIVE
    URLS "${H2O_SOURCE_URL}"
    FILENAME "h2o-${H2O_VERSION}.tar.gz"
    SHA512 f2f28905c01782a0432c9dfdb2f21054e0a4741ac4c5f26802d4b439d0172840aa215aba5dc7c9af62275dcc24de105674a3819384dc38246e43ce3e8263eb20
)

vcpkg_extract_source_archive(SOURCE_PATH
    ARCHIVE "${DIST_ARCHIVE}"
)

# --- Patch CMakeLists.txt -------------------------------------------------
file(READ "${SOURCE_PATH}/CMakeLists.txt" H2O_CMAKE_CONTENT)
if(NOT H2O_CMAKE_CONTENT MATCHES "VCPKG_H2O_PATCHED")
    string(REPLACE
        "find_package(OpenSSL REQUIRED"
        "set(VCPKG_H2O_PATCHED 1)\nfind_package(ZLIB REQUIRED)\nfind_package(OpenSSL REQUIRED"
        H2O_CMAKE_CONTENT "${H2O_CMAKE_CONTENT}")
    string(REPLACE "-Wno-unused-value"   "" H2O_CMAKE_CONTENT "${H2O_CMAKE_CONTENT}")
    string(REPLACE "-Wno-unused-function" "" H2O_CMAKE_CONTENT "${H2O_CMAKE_CONTENT}")
    string(REPLACE "-pthread" "" H2O_CMAKE_CONTENT "${H2O_CMAKE_CONTENT}")
    file(WRITE "${SOURCE_PATH}/CMakeLists.txt" "${H2O_CMAKE_CONTENT}")
endif()

# --- Provide pthread + POSIX + atomic shim for Windows -------------------------
# h2o unconditionally uses pthread_mutex_*, GCC atomic intrinsics,
# and POSIX headers (unistd.h, sys/uio.h) that don't exist on MSVC.
set(COMPAT_H "
#ifndef H2O_COMPAT_H
#define H2O_COMPAT_H

#ifdef _WIN32
#include <windows.h>

/*** pthread mutex shim ***/
#define pthread_mutex_t          CRITICAL_SECTION
#define pthread_mutex_init(m, a) InitializeCriticalSection(m)
#define pthread_mutex_lock(m)    EnterCriticalSection(m)
#define pthread_mutex_unlock(m)  LeaveCriticalSection(m)
#define pthread_mutex_destroy(m) DeleteCriticalSection(m)
#define pthread_self()           GetCurrentThreadId()
#define pthread_equal(a, b)     ((a) == (b))
typedef DWORD pthread_t;

/*** POSIX shim (unistd.h / sys/uio.h) ***/
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#include <io.h>
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/*** GCC atomic intrinsics shim - use volatile to suppress MSVC warnings ***/
#ifdef _M_X64
#include <intrin.h>
static __inline long long __sync_fetch_and_add_ll(volatile long long* ptr, long long val) {
    return _InterlockedExchangeAdd64(ptr, val);
}
static __inline long long __sync_fetch_and_sub_ll(volatile long long* ptr, long long val) {
    return _InterlockedExchangeAdd64(ptr, -val);
}
static __inline long long __sync_add_and_fetch_ll(volatile long long* ptr, long long val) {
    return _InterlockedExchangeAdd64(ptr, val) + val;
}
static __inline long long __sync_sub_and_fetch_ll(volatile long long* ptr, long long val) {
    return _InterlockedExchangeAdd64(ptr, -val) + (-val);
}
static __inline long __sync_fetch_and_add(long* ptr, long val) {
    return _InterlockedExchangeAdd(ptr, val);
}
static __inline long __sync_fetch_and_sub(long* ptr, long val) {
    return _InterlockedExchangeAdd(ptr, -val);
}
static __inline long __sync_add_and_fetch(long* ptr, long val) {
    return _InterlockedExchangeAdd(ptr, val) + val;
}
static __inline long __sync_sub_and_fetch(long* ptr, long val) {
    return _InterlockedExchangeAdd(ptr, -val) + (-val);
}
#define __sync_fetch_and_add(ptr, val)     __sync_fetch_and_add_ll((volatile long long*)(ptr), (long long)(val))
#define __sync_fetch_and_sub(ptr, val)     __sync_fetch_and_sub_ll((volatile long long*)(ptr), (long long)(val))
#define __sync_add_and_fetch(ptr, val)      __sync_add_and_fetch_ll((volatile long long*)(ptr), (long long)(val))
#define __sync_sub_and_fetch(ptr, val)      __sync_sub_and_fetch_ll((volatile long long*)(ptr), (long long)(val))
#endif

/*** Additional POSIX types needed ***/
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_IFMT
#define S_IFMT   0xF000
#endif

#endif /* _WIN32 */
#endif /* H2O_COMPAT_H */
")

# Write the compat header into every directory containing .c files that need it
file(WRITE "${SOURCE_PATH}/lib/common/h2o_compat.h"           "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/lib/core/h2o_compat.h"           "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/lib/http1/h2o_compat.h"          "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/lib/http2/h2o_compat.h"          "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/lib/http3/h2o_compat.h"          "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/lib/fastcgi/h2o_compat.h"        "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/lib/proxy/h2o_compat.h"           "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/deps/libgkc/h2o_compat.h"         "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/deps/libyrmcds/h2o_compat.h"      "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/deps/picohttpparser/h2o_compat.h" "${COMPAT_H}")
file(WRITE "${SOURCE_PATH}/deps/h2o_compat.h"                "${COMPAT_H}")

# Replace #include <pthread.h>, #include <unistd.h>, #include <sys/uio.h>
# with the h2o_compat.h shim
file(GLOB H2O_SRC_FILES
    "${SOURCE_PATH}/lib/common/*.c"
    "${SOURCE_PATH}/lib/core/*.c"
    "${SOURCE_PATH}/lib/http1/*.c"
    "${SOURCE_PATH}/lib/http2/*.c"
    "${SOURCE_PATH}/lib/http3/*.c"
    "${SOURCE_PATH}/lib/fastcgi/*.c"
    "${SOURCE_PATH}/lib/proxy/*.c"
    "${SOURCE_PATH}/deps/libgkc/*.c"
    "${SOURCE_PATH}/deps/libyrmcds/*.c"
    "${SOURCE_PATH}/deps/picohttpparser/*.c"
)
foreach(SRC ${H2O_SRC_FILES})
    file(READ "${SRC}" H2O_SRC_CONTENT)
    if(H2O_SRC_CONTENT MATCHES "#include <pthread.h>")
        string(REPLACE "#include <pthread.h>"
            "#include \"h2o_compat.h\""
            H2O_SRC_CONTENT "${H2O_SRC_CONTENT}")
    endif()
    if(H2O_SRC_CONTENT MATCHES "#include <unistd.h>")
        string(REPLACE "#include <unistd.h>"
            "#include \"h2o_compat.h\""
            H2O_SRC_CONTENT "${H2O_SRC_CONTENT}")
    endif()
    if(H2O_SRC_CONTENT MATCHES "#include <sys/uio.h>")
        string(REPLACE "#include <sys/uio.h>"
            "#include \"h2o_compat.h\""
            H2O_SRC_CONTENT "${H2O_SRC_CONTENT}")
    endif()
    if(H2O_SRC_CONTENT MATCHES "#include <sys/types.h>")
        string(REPLACE "#include <sys/types.h>"
            "#include \"h2o_compat.h\"\n#include <sys/types.h>"
            H2O_SRC_CONTENT "${H2O_SRC_CONTENT}")
    endif()
    file(WRITE "${SRC}" "${H2O_SRC_CONTENT}")
endforeach()

# Also patch the h2o header files (include/h2o/*.h) that reference unistd.h / pthread.h
file(GLOB H2O_HEADER_FILES "${SOURCE_PATH}/include/h2o/*.h")
foreach(HDR ${H2O_HEADER_FILES})
    file(READ "${HDR}" H2O_HDR_CONTENT)
    if(H2O_HDR_CONTENT MATCHES "#include <unistd.h>")
        string(REPLACE "#include <unistd.h>"
            "#ifdef _WIN32\n#include <io.h>\n#else\n#include <unistd.h>\n#endif"
            H2O_HDR_CONTENT "${H2O_HDR_CONTENT}")
        file(WRITE "${HDR}" "${H2O_HDR_CONTENT}")
    endif()
endforeach()

# --- Configure and build ---------------------------------------------------
vcpkg_configure_cmake(
    SOURCE_PATH "${SOURCE_PATH}"
    GENERATOR Ninja
    OPTIONS
        -DWITH_OPENSSL=ON
        -DWITH_LIBUV=ON
        -DWITH_WEPOLL=OFF
        -DWITH_MRUBY=OFF
        -DWITH_BUNDED_SSL=OFF
        -DWITH_BUNDLED_ZLIB=OFF
        -DBUILD_SHARED_LIBS=ON
    OPTIONS_DEBUG
        -DCMAKE_DEBUG_POSTFIX=d
)

vcpkg_install_cmake()

# --- Fix installed CMake targets -------------------------------------------
if(WIN32)
    file(READ "${CURRENT_PACKAGES_DIR}/share/h2o/h2oTargets.cmake" H2O_TARGETS)
    if(NOT H2O_TARGETS MATCHES "ws2_32")
        string(REPLACE "INTERFACE_LINK_LIBRARIES"
            "INTERFACE_LINK_LIBRARIES \"ws2_32\""
            H2O_TARGETS "${H2O_TARGETS}")
        file(WRITE "${CURRENT_PACKAGES_DIR}/share/h2o/h2oTargets.cmake" "${H2O_TARGETS}")
    endif()
endif()

vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/h2o TARGET_PATH share/h2o)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
