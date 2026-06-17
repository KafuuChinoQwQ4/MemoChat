# MediaService owns the media domain code (relocated from GateShared in slice j).
# GateMediaCore (media storage local+S3, Http2MediaSupport) and GateMediaDomain
# (media route module + service facade); target names unchanged.

# --- GateMediaCore: media storage (local + S3) and media support. ------------
add_library(GateMediaCore STATIC
    core/storage/MediaStorage.cpp
    core/storage/MediaStorage.h
    core/storage/S3MediaStorage.cpp
    core/storage/S3MediaStorage.h
    core/support/Http2MediaSupport.cpp
    core/support/Http2MediaSupport.h
)
memochat_configure_gate_core_lib(GateMediaCore)
target_include_directories(GateMediaCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/core/support
)
target_link_libraries(GateMediaCore PUBLIC GateInfraCore)

# --- GateMediaDomain: media upload/download routes + service facade. ----------
add_library(GateMediaDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.h
    domain/profiles/GateMediaProfile.cpp
    domain/modules/media/MediaRouteModule.cpp
    domain/services/media/MediaService.cpp
    domain/MediaHttpService.cpp
)
memochat_configure_gate_app_lib(GateMediaDomain)
target_include_directories(GateMediaDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateMediaDomain PUBLIC GateRuntimeCore GateMediaCore)

add_executable(MediaGatewayServer
    app/MediaGatewayServer.cpp
)
memochat_configure_gate_focused_service(MediaGatewayServer DOMAINS GateMediaDomain)
