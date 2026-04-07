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
    target_link_options(VarifyServer PRIVATE
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64"
        "/LIBPATH:D:/VS2026stable/VC/Tools/MSVC/14.50.35717/lib/x64")
    target_link_libraries(VarifyServer PRIVATE
        "gdi32" "user32")

    # Override: config.ini lives in source dir, not in RUNTIME_OUTPUT_DIRECTORY.
    # After the auto-deploy from memochat_configure_server_target, copy from source.
    set(RUNTIME_DIR "D:\\MemoChat-Qml-Drogon\\Memo_ops\\runtime\\services\\VarifyServer")
    add_custom_command(TARGET VarifyServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
            "${RUNTIME_DIR}\\"
        COMMENT "Deploying config.ini -> ${RUNTIME_DIR}\\"
    )
endif()
