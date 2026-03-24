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
    config.ini
)

set_target_properties(StatusServer PROPERTIES FOLDER "Server")
memochat_configure_server_target(StatusServer)
target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_PQXX_TARGET})
if(WIN32)
    target_link_options(StatusServer PRIVATE
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64"
        "/LIBPATH:C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64"
        "/LIBPATH:D:/VS2026stable/VC/Tools/MSVC/14.50.35717/lib/x64")
endif()
if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()
if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(StatusServer PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
endif()
