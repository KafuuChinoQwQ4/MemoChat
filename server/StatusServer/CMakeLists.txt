add_executable(StatusServer
    AsioIOServicePool.cpp
    ChatGrpcClient.cpp
    ConfigMgr.cpp
    DistLock.cpp
    MysqlDao.cpp
    MysqlMgr.cpp
    RedisMgr.cpp
    StatusServer.cpp
    StatusServiceImpl.cpp
    AsioIOServicePool.h
    ChatGrpcClient.h
    ConfigMgr.h
    const.h
    DistLock.h
    MysqlDao.h
    MysqlMgr.h
    RedisMgr.h
    Singleton.h
    StatusServiceImpl.h
    message.proto
    config.ini
)

memochat_add_proto_sources(StatusServer message.proto)
set_target_properties(StatusServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(StatusServer)
