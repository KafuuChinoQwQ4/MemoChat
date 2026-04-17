add_executable(AIServer
    AIServer.cpp
    AIServiceImpl.cpp
    AIServiceCore.cpp
    AIServiceClient.cpp
    ConfigMgr.cpp
    ConversationContext.cpp
    db/AISessionRepo.cpp
    db/AISmartLogRepo.cpp
)

set_target_properties(AIServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(AIServer)
target_link_libraries(AIServer PRIVATE ${MEMOCHAT_PQXX_TARGET})
if(WIN32)
    # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
    target_link_options(AIServer PRIVATE
        "/LIBPATH:$ENV{WINSDK_LIB_UM}"
        "/LIBPATH:$ENV{WINSDK_LIB_UCRT}"
        "/LIBPATH:$ENV{VC_TOOLS_LIB}")
endif()
