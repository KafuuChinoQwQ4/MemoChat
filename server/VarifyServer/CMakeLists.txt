add_executable(VarifyServer
    VarifyServer.cpp
    VarifyServiceImpl.cpp
    VarifyRedisMgr.cpp
    RateLimiter.cpp
    EmailSender.cpp
    EmailTaskBus.cpp
)

set_target_properties(VarifyServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(VarifyServer)
target_link_libraries(VarifyServer PRIVATE memochat_boost_stubs GateServerCore)

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

    # Override: config.ini lives in source dir, not in RUNTIME_OUTPUT_DIRECTORY.
    # After the auto-deploy from memochat_configure_server_target, copy from source.
    # Environment variable: MEMOCHAT_RUNTIME_BASE (defaults to ${sourceDir}/Memo_ops/runtime/services)
    if(DEFINED ENV{MEMOCHAT_RUNTIME_BASE})
        file(TO_CMAKE_PATH "$ENV{MEMOCHAT_RUNTIME_BASE}/VarifyServer" RUNTIME_DIR_NATIVE)
    else()
        file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/Memo_ops/runtime/services/VarifyServer" RUNTIME_DIR_NATIVE)
    endif()
    add_custom_command(TARGET VarifyServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
            "${RUNTIME_DIR_NATIVE}"
        COMMENT "Deploying config.ini -> ${RUNTIME_DIR_NATIVE}"
    )
endif()
