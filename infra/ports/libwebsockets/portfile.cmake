# Overlay port: pin libwebsockets to upstream main with HTTP/3 WebTransport APIs.
#
# vcpkg's builtin 4.5.8 port does not expose the WebTransport role header/API
# needed by ChatServer's optional provider. This commit is the audited upstream
# main HEAD from 2026-06-30 and contains lws-webtransport.h,
# lws_wt_create_stream(), and lws_wt_is_session().
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO warmcat/libwebsockets
    REF 7316933aefbc10369251c3b6717b166c3f7a86ca
    SHA512 0f6a41eba7c763c0b141b4cf5c29b848145d41cfe9be9664531a55c3d6f04dda512355548e5a83589c3ec41e5a151247712c110ef0ca64873b875b3aa52eb333
    HEAD_REF main
    PATCHES
        patches/enable-h3-webtransport-settings.patch
)

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" LWS_WITH_STATIC)
string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "dynamic" LWS_WITH_SHARED)

set(EXTRA_ARGS)
if(NOT VCPKG_TARGET_ARCHITECTURE STREQUAL "wasm32")
    list(APPEND EXTRA_ARGS -DLWS_WITH_LIBUV=ON)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${EXTRA_ARGS}
        -DLWS_WITH_STATIC=${LWS_WITH_STATIC}
        -DLWS_WITH_SHARED=${LWS_WITH_SHARED}
        -DLWS_WITH_SSL=ON
        -DLWS_WITH_GNUTLS=ON
        -DLWS_WITH_HTTP3=ON
        -DLWS_ROLE_QUIC=ON
        -DLWS_ROLE_WT=ON
        -DLWS_WITH_HTTP2=ON
        -DLWS_ROLE_H1=ON
        -DLWS_ROLE_WS=ON
        -DLWS_WITH_GENCRYPTO=ON
        -DLWS_WITH_BUNDLED_ZLIB=OFF
        -DLWS_WITH_HTTP_STREAM_COMPRESSION=ON
        -DLWS_IPV6=ON
        -DLWS_WITH_EXTERNAL_POLL=ON
        -DLWS_WITH_MINIMAL_EXAMPLES=OFF
        -DLWS_WITHOUT_TESTAPPS=ON
)

vcpkg_cmake_install()

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_cmake_config_fixup(CONFIG_PATH cmake)
else()
    vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/libwebsockets)
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/libwebsockets-test-server")

if(EXISTS "${CURRENT_PACKAGES_DIR}/share/libwebsockets/libwebsockets-config.cmake")
    file(READ "${CURRENT_PACKAGES_DIR}/share/libwebsockets/libwebsockets-config.cmake" LIBWEBSOCKETSCONFIG_CMAKE)
    string(REPLACE "/../include" "/../../include" LIBWEBSOCKETSCONFIG_CMAKE "${LIBWEBSOCKETSCONFIG_CMAKE}")
    file(WRITE "${CURRENT_PACKAGES_DIR}/share/libwebsockets/libwebsockets-config.cmake" "${LIBWEBSOCKETSCONFIG_CMAKE}")
endif()

if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    vcpkg_replace_string(
        "${CURRENT_PACKAGES_DIR}/share/libwebsockets/LibwebsocketsTargets-debug.cmake"
        "websockets_static.lib"
        "websockets.lib"
        IGNORE_UNCHANGED
    )
endif()

if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    vcpkg_replace_string(
        "${CURRENT_PACKAGES_DIR}/share/libwebsockets/LibwebsocketsTargets-release.cmake"
        "websockets_static.lib"
        "websockets.lib"
        IGNORE_UNCHANGED
    )
endif()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static" AND VCPKG_TARGET_IS_WINDOWS)
    file(RENAME "${CURRENT_PACKAGES_DIR}/debug/lib/websockets_static.lib" "${CURRENT_PACKAGES_DIR}/debug/lib/websockets.lib")
    file(RENAME "${CURRENT_PACKAGES_DIR}/lib/websockets_static.lib" "${CURRENT_PACKAGES_DIR}/lib/websockets.lib")
endif()

vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()

if(EXISTS "${CURRENT_PACKAGES_DIR}/include/lws_config.h")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/lws_config.h" "${CURRENT_PACKAGES_DIR}" "")
endif()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
