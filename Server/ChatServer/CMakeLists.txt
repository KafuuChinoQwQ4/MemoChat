# 定义 ChatServer 项目
project(ChatServer)

# 包含路径 (复用 GateServer 的部分头文件路径)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServer
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/_deps/jsoncpp-src/include
)

# 源文件列表
set(CHAT_SERVER_SOURCES
    main.cpp
    CServer.cpp
    CSession.cpp
    LogicSystem.cpp
    # 复用 GateServer 的通用类 (建议直接拷贝过来，或者在这里引用)
    ../GateServer/ConfigMgr.cpp
    ../GateServer/AsioIOServicePool.cpp
    ../GateServer/RedisMgr.cpp
    ../GateServer/MysqlMgr.cpp
    ../GateServer/MysqlDao.cpp
)

add_executable(ChatServer ${CHAT_SERVER_SOURCES})

# 链接库
target_link_libraries(ChatServer PRIVATE 
    Boost::system 
    Boost::thread
    jsoncpp_lib
    mysqlcppconn.lib
    hiredis.lib
    ws2_32
)

# 配置文件复制策略
target_sources(ChatServer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")

add_custom_target(CopyChatConfig ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        "$<TARGET_FILE_DIR:ChatServer>/config.ini"
    COMMENT "Updating ChatServer config.ini..."
)

# 【关键修改】让 Copy 任务依赖于 ChatServer，而不是反过来
# 之前写成了 add_dependencies(ChatServer CopyChatConfig) -> 导致死循环
add_dependencies(CopyChatConfig ChatServer)