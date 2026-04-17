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
target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_PQXX_TARGET})
if(WIN32)
    # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
    target_link_options(StatusServer PRIVATE
        "/LIBPATH:$ENV{WINSDK_LIB_UM}"
        "/LIBPATH:$ENV{WINSDK_LIB_UCRT}"
        "/LIBPATH:$ENV{VC_TOOLS_LIB}")
endif()
if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()
if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
endif()
