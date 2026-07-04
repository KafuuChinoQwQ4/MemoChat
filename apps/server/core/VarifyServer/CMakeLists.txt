add_library(VarifyServerCore STATIC
    VerifyCodePolicy.cpp
    EmailDeliveryTaskCodec.cpp
)
target_compile_features(VarifyServerCore PUBLIC cxx_std_23)
target_include_directories(VarifyServerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(VarifyServerCore PUBLIC memochat_server_deps)
if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(VarifyServerCore
        MODULES
            memochat.varify.email_delivery_task_algorithms=cxx_modules/EmailDeliveryTask.cppm
            memochat.varify.verify_code_algorithms=cxx_modules/VerifyCode.cppm
        CONSUMER_SOURCES
            EmailDeliveryTaskCodec.cpp
            VerifyCodePolicy.cpp
    )
endif()

add_executable(VarifyServer
    VarifyServer.cpp
    VarifyServiceImpl.cpp
    VarifyRedisMgr.cpp
    RateLimiter.cpp
    EmailSender.cpp
    EmailTaskBus.cpp
    ConfigMgr.cpp
)

set_target_properties(VarifyServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(VarifyServer)
# VarifyServer owns its own thin ConfigMgr now, so it no longer links the whole
# GateServerCore static lib (it only ever needed ConfigMgr from it). This is the
# Phase 1 "sever VarifyServer -> GateServerCore" step of the gateserver split.
target_link_libraries(VarifyServer PRIVATE VarifyServerCore memochat_boost_stubs)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(VarifyServer
        MODULES
            memochat.varify.email_sender_algorithms=cxx_modules/EmailSender.cppm
            memochat.varify.email_task_bus_algorithms=cxx_modules/EmailTaskBus.cppm
            memochat.varify.rate_limiter_algorithms=cxx_modules/RateLimiter.cppm
            memochat.varify.redis_algorithms=cxx_modules/VarifyRedis.cppm
            memochat.varify.server_runtime_algorithms=cxx_modules/VarifyServerRuntime.cppm
            memochat.varify.service_algorithms=cxx_modules/VarifyService.cppm
        CONSUMER_SOURCES
            EmailSender.cpp
            EmailTaskBus.cpp
            RateLimiter.cpp
            VarifyRedisMgr.cpp
            VarifyServer.cpp
            VarifyServiceImpl.cpp
    )
endif()

# ── Embed shared Lua scripts ─────────────────────────────────────────────────
include(EmbedLuaScripts)
memochat_embed_lua_scripts(
    SCRIPTS
        "${CMAKE_SOURCE_DIR}/apps/server/core/common/lua/incr_with_expire.lua"
    OUTPUT_DIR  "${CMAKE_BINARY_DIR}/generated/lua/common"
    HEADER_NAME "common_lua_scripts.hpp"
    NAMESPACE   "memochat::common::lua_scripts"
)
target_include_directories(VarifyServer PRIVATE "${CMAKE_BINARY_DIR}/generated/lua/common")

if(WIN32)
    # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
    if(DEFINED ENV{WINSDK_LIB_UM} AND NOT "$ENV{WINSDK_LIB_UM}" STREQUAL "")
        set(_WINSDK_LIB_UM "$ENV{WINSDK_LIB_UM}")
    else()
        set(_WINSDK_LIB_UM "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64")
    endif()
    if(DEFINED ENV{WINSDK_LIB_UCRT} AND NOT "$ENV{WINSDK_LIB_UCRT}" STREQUAL "")
        set(_WINSDK_LIB_UCRT "$ENV{WINSDK_LIB_UCRT}")
    else()
        set(_WINSDK_LIB_UCRT "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/ucrt/x64")
    endif()
    target_link_options(VarifyServer PRIVATE
        "/LIBPATH:${_WINSDK_LIB_UM}"
        "/LIBPATH:${_WINSDK_LIB_UCRT}")
    target_link_libraries(VarifyServer PRIVATE
        "gdi32" "user32")

    if(MEMOCHAT_AUTO_DEPLOY_RUNTIME)
        # Override: config.ini lives in source dir, not in RUNTIME_OUTPUT_DIRECTORY.
        # After the auto-deploy from memochat_configure_server_target, copy from source.
        # Environment variable: MEMOCHAT_RUNTIME_BASE (defaults to ${sourceDir}/infra/Memo_ops/runtime/services)
        if(DEFINED ENV{MEMOCHAT_RUNTIME_BASE})
            file(TO_CMAKE_PATH "$ENV{MEMOCHAT_RUNTIME_BASE}/VarifyServer" RUNTIME_DIR_NATIVE)
        else()
            file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/infra/Memo_ops/runtime/services/VarifyServer" RUNTIME_DIR_NATIVE)
        endif()
        add_custom_command(TARGET VarifyServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
                "${RUNTIME_DIR_NATIVE}"
            COMMENT "Deploying config.ini -> ${RUNTIME_DIR_NATIVE}"
        )
    endif()
endif()
