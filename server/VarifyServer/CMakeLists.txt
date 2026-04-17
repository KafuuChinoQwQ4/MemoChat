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
target_link_libraries(VarifyServer PRIVATE GateServerCore)

if(WIN32)
    # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
    target_link_options(VarifyServer PRIVATE
        "/LIBPATH:$ENV{WINSDK_LIB_UM}"
        "/LIBPATH:$ENV{WINSDK_LIB_UCRT}"
        "/LIBPATH:$ENV{VC_TOOLS_LIB}")
    target_link_libraries(VarifyServer PRIVATE
        "gdi32" "user32")

    # Override: config.ini lives in source dir, not in RUNTIME_OUTPUT_DIRECTORY.
    # After the auto-deploy from memochat_configure_server_target, copy from source.
    # Environment variable: MEMOCHAT_RUNTIME_BASE (defaults to ${sourceDir}/Memo_ops/runtime/services)
    if(DEFINED ENV{MEMOCHAT_RUNTIME_BASE})
        set(RUNTIME_DIR "$ENV{MEMOCHAT_RUNTIME_BASE}\\VarifyServer")
    else()
        set(RUNTIME_DIR "${CMAKE_SOURCE_DIR}/Memo_ops/runtime/services/VarifyServer")
    endif()
    add_custom_command(TARGET VarifyServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
            "${RUNTIME_DIR}\\"
        COMMENT "Deploying config.ini -> ${RUNTIME_DIR}\\"
    )
endif()
