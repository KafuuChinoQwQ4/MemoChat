# overlays/nghttp2/portfile.cmake
# Build nghttp2 as a static library from the pre-downloaded tarball.

set(VCPKG_DOWNLOADS_DIR "$ENV{VCPKG_DOWNLOADS}")
if(NOT VCPKG_DOWNLOADS_DIR)
    set(VCPKG_DOWNLOADS_DIR "D:/vcpkg/downloads")
endif()
set(TARBALL "${VCPKG_DOWNLOADS_DIR}/nghttp2-nghttp2-v1.68.0.tar.gz")

if(NOT EXISTS "${TARBALL}")
    message(FATAL_ERROR "nghttp2 tarball not found at ${TARBALL}")
endif()

message(STATUS "Using nghttp2 tarball: ${TARBALL}")

# Remove any stale extraction marker so vcpkg re-extracts
file(REMOVE "${CURRENT_BUILDTREES_DIR}/src/nghttp2-nghttp2-v1.68.0.tar.gz.extracted")

# Extract the archive — vcpkg_extract_source_archive sets SOURCE_PATH
vcpkg_extract_source_archive(OUT_SOURCE_PATH ARCHIVE "${TARBALL}")
message(STATUS "nghttp2 source extracted to: ${OUT_SOURCE_PATH}")

# Configure — build only the static library (no DLL).
# nghttp2 v1.68 uses BUILD_SHARED_LIBS and BUILD_STATIC_LIBS (not ENABLE_*).
# Must override vcpkg's default BUILD_SHARED_LIBS=ON via MAYBE_UNUSED_VARIABLES
# to suppress the warning about unused options.
vcpkg_configure_cmake(
    SOURCE_PATH "${OUT_SOURCE_PATH}"
    GENERATOR Ninja
    OPTIONS
        -DENABLE_EXAMPLES=OFF
        -DENABLE_FAILMALLOC=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_STATIC_LIBS=ON
    MAYBE_UNUSED_VARIABLES
        BUILD_SHARED_LIBS
        BUILD_STATIC_LIBS
)

# Build and install the library
vcpkg_install_cmake()

# Install copyright file (required by vcpkg policy).
# Use CURRENT_PORT_DIR which vcpkg sets to the port's root directory.
file(INSTALL "${CURRENT_PORT_DIR}/COPYING"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
     RENAME copyright)

# Fix up CMake targets
# nghttp2's CMake exports to lib/cmake/nghttp2
vcpkg_fixup_cmake_targets(
    TARGET_PATH "share/nghttp2"
    CONFIG_PATH "lib/cmake/nghttp2"
)
