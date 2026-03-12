add_executable(GateServer
    AsioIOServicePool.cpp
    CallService.cpp
    ChatGrpcClient.cpp
    ConfigMgr.cpp
    CServer.cpp
    GateServer.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    MediaStorage.cpp
    MysqlDao.cpp
    MysqlMgr.cpp
    RedisMgr.cpp
    StatusGrpcClient.cpp
    VerifyGrpcClient.cpp
    AsioIOServicePool.h
    CallService.h
    ChatGrpcClient.h
    ConfigMgr.h
    const.h
    CServer.h
    HttpConnection.h
    LogicSystem.h
    MediaStorage.h
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
if(WIN32)
    target_link_libraries(GateServer PRIVATE bcrypt)
endif()
