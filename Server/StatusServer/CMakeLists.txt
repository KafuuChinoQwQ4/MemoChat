# StatusServer/CMakeLists.txt (示例)
project(StatusServer)

# 包含头文件路径 (根据你的项目结构调整)
include_directories(${CMAKE_SOURCE_DIR}/Server/StatusServer)
# ... 其他 include 如 RedisMgr, ConfigMgr, MySQL 等 ...

# 源文件
file(GLOB SRC_FILES "*.cpp" "*.cc" "../GateServer/ConfigMgr.cpp" "../GateServer/RedisMgr.cpp" "../GateServer/MysqlMgr.cpp" "../GateServer/MysqlDao.cpp")

add_executable(StatusServer ${SRC_FILES})

# 链接库 (gRPC, protobuf, mysql, boost等)
target_link_libraries(StatusServer
    PRIVATE
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    mysqlcppconn
    hiredis
    Boost::system
    Boost::filesystem
)