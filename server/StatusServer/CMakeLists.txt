add_executable(StatusServer
    AsioIOServicePool.cpp
    ChatGrpcClient.cpp
    ConfigMgr.cpp
    DistLock.cpp
    PostgresDao.cpp
    PostgresMgr.cpp
    RedisMgr.cpp
    StatusAsyncSideEffects.cpp
    StatusServer.cpp
    StatusServiceImpl.cpp
    AsioIOServicePool.h
    ChatGrpcClient.h
    ConfigMgr.h
    const.h
    DistLock.h
    PostgresDao.h
    PostgresMgr.h
    RedisMgr.h
    Singleton.h
    StatusAsyncSideEffects.h
    StatusServiceImpl.h
)

set_target_properties(StatusServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(StatusServer)
target_link_libraries(StatusServer PRIVATE memochat_boost_stubs ${MEMOCHAT_PQXX_TARGET})
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
    target_link_options(StatusServer PRIVATE
        "/LIBPATH:${_WINSDK_LIB_UM}"
        "/LIBPATH:${_WINSDK_LIB_UCRT}")
endif()
if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()
if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
endif()
