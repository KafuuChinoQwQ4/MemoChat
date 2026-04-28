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
target_link_libraries(AIServer PRIVATE memochat_boost_stubs ${MEMOCHAT_PQXX_TARGET})
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
    target_link_options(AIServer PRIVATE
        "/LIBPATH:${_WINSDK_LIB_UM}"
        "/LIBPATH:${_WINSDK_LIB_UCRT}")
endif()
