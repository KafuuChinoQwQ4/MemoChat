# MediaService owns the media domain code (relocated from GateShared in slice j).
# GateMediaCore (media storage local+S3, Http2MediaSupport) and GateMediaDomain
# (media route module + service facade); target names unchanged.

# --- GateMediaCore: media storage (local + S3) and media support. ------------
add_library(GateMediaCore STATIC
    domain/services/media/MediaPublicDtos.cpp
    domain/services/media/MediaPublicDtos.hpp
    domain/services/media/MediaUploadSessionDto.cpp
    domain/services/media/MediaUploadSessionDto.hpp
    core/storage/MediaStorage.cpp
    core/storage/MediaStorage.hpp
    core/storage/S3MediaStorage.cpp
    core/storage/S3MediaStorage.hpp
    core/support/Http2MediaSupport.cpp
    core/support/Http2MediaSupport.hpp
)
memochat_configure_gate_core_lib(GateMediaCore)
target_include_directories(GateMediaCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/core/support
    ${CMAKE_CURRENT_SOURCE_DIR}/domain
)
target_link_libraries(GateMediaCore PUBLIC GateInfraCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateMediaCore
        MODULES
            memochat.media.config_algorithms=core/cxx_modules/MediaConfig.cppm
            memochat.media.public_dto_algorithms=domain/services/media/cxx_modules/MediaPublicDto.cppm
            memochat.media.upload_session_algorithms=domain/services/media/cxx_modules/MediaUploadSession.cppm
            memochat.media.local_storage_algorithms=core/storage/cxx_modules/LocalMediaStorage.cppm
            memochat.media.s3_storage_algorithms=core/storage/cxx_modules/S3MediaStorage.cppm
        CONSUMER_SOURCES
            domain/services/media/MediaPublicDtos.cpp
            domain/services/media/MediaUploadSessionDto.cpp
            core/storage/MediaStorage.cpp
            core/storage/S3MediaStorage.cpp
            core/support/Http2MediaSupport.cpp
    )
endif()

# --- GateMediaDomain: media upload/download routes + service facade. ----------
add_library(GateMediaDomain STATIC
    ${MEMOCHAT_SERVER_CORE_DIR}/GateShared/GateRouteProfileRegistrar.hpp
    domain/profiles/GateMediaProfile.cpp
    domain/modules/media/MediaRouteModule.cpp
    domain/modules/media/MediaRouteSchemas.cpp
    domain/services/media/MediaPersistence.cpp
    domain/services/media/MediaPersistence.hpp
    domain/services/media/MediaService.cpp
    domain/MediaHttpService.cpp
)
memochat_configure_gate_app_lib(GateMediaDomain)
target_include_directories(GateMediaDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/domain)
target_link_libraries(GateMediaDomain PUBLIC GateRuntimeCore GateMediaCore)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(GateMediaDomain
        MODULES
            memochat.media.config_algorithms=core/cxx_modules/MediaConfig.cppm
            memochat.media.route_registration_algorithms=domain/modules/media/cxx_modules/MediaRouteRegistration.cppm
            memochat.media.route_schema_algorithms=domain/modules/media/cxx_modules/MediaRouteSchema.cppm
            memochat.media.service_algorithms=domain/services/media/cxx_modules/MediaService.cppm
        CONSUMER_SOURCES
            domain/modules/media/MediaRouteModule.cpp
            domain/modules/media/MediaRouteSchemas.cpp
            domain/services/media/MediaService.cpp
    )
endif()

add_executable(MediaGatewayServer
    app/MediaGatewayServer.cpp
)
memochat_configure_gate_focused_service(MediaGatewayServer DOMAINS GateMediaDomain)
