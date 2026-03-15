add_executable(GateServer
    AsioIOServicePool.cpp
    CallService.cpp
    ChatGrpcClient.cpp
    ConfigMgr.cpp
    CServer.cpp
    AuthLoginSupport.cpp
    GateHttpJsonSupport.cpp
    GateAsyncSideEffects.cpp
    GateRouteModules.cpp
    GateServer.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    MediaHttpService.cpp
    MediaStorage.cpp
    PostgresDao.cpp
    PostgresMgr.cpp
    RedisMgr.cpp
    StatusGrpcClient.cpp
    VerifyGrpcClient.cpp
    AsioIOServicePool.h
    CallService.h
    ChatGrpcClient.h
    ConfigMgr.h
    const.h
    CServer.h
    AuthLoginSupport.h
    GateHttpJsonSupport.h
    GateAsyncSideEffects.h
    GateRouteModules.h
    HttpConnection.h
    LogicSystem.h
    MediaHttpService.h
    MediaStorage.h
    PostgresDao.h
    PostgresMgr.h
    RedisMgr.h
    Singleton.h
    StatusGrpcClient.h
    VerifyGrpcClient.h
    config.ini
)

set_target_properties(GateServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServer)
target_link_libraries(GateServer PRIVATE ${MEMOCHAT_PQXX_TARGET})
if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(GateServer PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()
if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(GateServer PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
endif()
if(WIN32)
    target_link_libraries(GateServer PRIVATE bcrypt)
endif()
