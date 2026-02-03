# ==========================================
# ChatServer - CMake 配置 (手动路径版)
# ==========================================

# 1. 定义依赖库路径 (与 GateServer 保持一致)
set(GRPC_DIR "D:/cppsoft/grpc/install")
set(REDIS_DIR "D:/cppsoft/redis")
set(MYSQL_DIR "D:/cppsoft/mysql_connector")
set(BOOST_ROOT_DIR "D:/cppsoft/boost_1_90_0")
set(BOOST_LIB_DIR "D:/cppsoft/boost_1_90_0/lib64-msvc-14.3")

# 2. 头文件路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServer  # 引用 GateServer 的头文件
    "${BOOST_ROOT_DIR}"
    "${GRPC_DIR}/include"
    "${CMAKE_BINARY_DIR}/_deps/jsoncpp-src/include"
    "${REDIS_DIR}/include"
    "${MYSQL_DIR}/include"
    "${MYSQL_DIR}/include/jdbc"
)

# 3. 库文件路径 (让链接器能找到 .lib)
link_directories(
    "${GRPC_DIR}/lib"
    "${BOOST_LIB_DIR}"
    "${REDIS_DIR}/lib"
    "${MYSQL_DIR}/lib64/vs14"
)

# 4. 自动查找 gRPC 依赖库
file(GLOB ABSL_LIBS "${GRPC_DIR}/lib/absl_*.lib")
file(GLOB UPB_LIBS "${GRPC_DIR}/lib/upb_*.lib")
file(GLOB UTF8_LIBS "${GRPC_DIR}/lib/utf8_*.lib")
file(GLOB ADDR_SORT_LIB "${GRPC_DIR}/lib/address_sorting.lib")
file(GLOB CARES_LIB "${GRPC_DIR}/lib/cares.lib")

# 5. 复用 GateServer 的通用代码
# 注意：这里直接编译源文件，避免复杂的库依赖
set(SHARED_SRCS
    ../GateServer/ConfigMgr.cpp
    ../GateServer/RedisMgr.cpp
    ../GateServer/MysqlMgr.cpp
    ../GateServer/MysqlDao.cpp
    ../GateServer/AsioIOServicePool.cpp
    ../GateServer/VerifyGrpcClient.cpp
)

# 6. ChatServer 自己的源文件
file(GLOB SERVER_SOURCES "*.cpp" "*.cc")

# 7. gRPC 代码生成 (如果尚未生成)
# 也可以直接引用 ../GateServer/message.pb.cc 如果它们是共用的
# 但为了独立性，这里我们假设 ChatServer 也要处理 proto
# 如果想复用 GateServer 的生成文件，可以将下面几行注释掉，并在 SERVER_SOURCES 里加入路径
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../GateServer/message.proto")
set(PROTOC "${GRPC_DIR}/bin/protoc.exe")
set(GRPC_CPP_PLUGIN "${GRPC_DIR}/bin/grpc_cpp_plugin.exe")

set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/message.pb.cc" "${CMAKE_CURRENT_BINARY_DIR}/message.grpc.pb.cc")
set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/message.pb.h" "${CMAKE_CURRENT_BINARY_DIR}/message.grpc.pb.h")

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND ${PROTOC}
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         -I "${CMAKE_CURRENT_SOURCE_DIR}/../GateServer"
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC files for ChatServer"
)

# 8. 生成可执行文件
add_executable(ChatServer ${SERVER_SOURCES} ${SHARED_SRCS} ${PROTO_SRCS})

# 9. 编译选项
if (MSVC)
    add_compile_options("/utf-8")
endif()

target_compile_definitions(ChatServer PRIVATE 
    BOOST_ERROR_CODE_HEADER_ONLY
    WIN32_LEAN_AND_MEAN
)

# 10. 链接库
# 注意：Boost 在 Windows MSVC 下通常会自动链接 (Auto-Linking)，
# 只要 link_directories 正确且包含头文件即可。
# Boost::system 是 Root 定义的接口库，主要传导 include 路径。
target_link_libraries(ChatServer PRIVATE 
    Boost::system 
    jsoncpp_lib
    
    libprotobuf 
    gpr 
    grpc 
    grpc++ 
    
    ${ABSL_LIBS}
    ${UPB_LIBS}
    ${UTF8_LIBS}
    ${ADDR_SORT_LIB}
    ${CARES_LIB}
    
    re2
    ssl 
    crypto 
    zlib
    
    ws2_32 
    crypt32
    mswsock

    hiredis.lib
    mysqlcppconn.lib
)

# 11. 配置文件自动复制
target_sources(ChatServer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")
add_custom_target(CopyChatServerConfig ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        "$<TARGET_FILE_DIR:ChatServer>/config.ini"
    COMMENT "Updating ChatServer config.ini..."
)
add_dependencies(CopyChatServerConfig ChatServer)