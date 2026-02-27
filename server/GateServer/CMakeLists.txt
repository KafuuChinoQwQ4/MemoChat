add_executable(GateServer
    AsioIOServicePool.cpp
    ConfigMgr.cpp
    CServer.cpp
    GateServer.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    MysqlDao.cpp
    MysqlMgr.cpp
    RedisMgr.cpp
    StatusGrpcClient.cpp
    VerifyGrpcClient.cpp
    AsioIOServicePool.h
    ConfigMgr.h
    const.h
    CServer.h
    HttpConnection.h
    LogicSystem.h
    MysqlDao.h
    MysqlMgr.h
    RedisMgr.h
    Singleton.h
    StatusGrpcClient.h
    VerifyGrpcClient.h
    message.proto
    config.ini
)

memochat_add_proto_sources(GateServer message.proto)
set_target_properties(GateServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServer)
