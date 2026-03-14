add_executable(StatusServer
    AsioIOServicePool.cpp
    ChatGrpcClient.cpp
    ConfigMgr.cpp
    DistLock.cpp
    PostgresDao.cpp
    PostgresMgr.cpp
    RedisMgr.cpp
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
    StatusServiceImpl.h
    config.ini
)

set_target_properties(StatusServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(StatusServer)
target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_PQXX_TARGET})
