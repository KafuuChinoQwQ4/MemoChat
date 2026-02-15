project(StatusServer)

# 1. 定义依赖库路径 (与 GateServer 保持一致)
set(GRPC_DIR "D:/cppsoft/grpc/install")
set(REDIS_DIR "D:/cppsoft/redis")
set(MYSQL_DIR "D:/cppsoft/mysql_connector")
set(BOOST_ROOT_DIR "D:/cppsoft/boost_1_90_0")

# 2. 头文件路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServer
    "${BOOST_ROOT_DIR}"
    "${GRPC_DIR}/include"
    "${CMAKE_BINARY_DIR}/_deps/jsoncpp-src/include"
    "${REDIS_DIR}/include"
    "${MYSQL_DIR}/include"
    "${MYSQL_DIR}/include/jdbc"
)

# 3. 库文件路径
link_directories(
    "${GRPC_DIR}/lib"
    "D:/cppsoft/boost_1_90_0/lib64-msvc-14.3"
    "${REDIS_DIR}/lib"
    "${MYSQL_DIR}/lib64/vs14"
)

# 4. === 自动查找所有依赖库 ===
file(GLOB ABSL_LIBS "${GRPC_DIR}/lib/absl_*.lib")
file(GLOB UPB_LIBS "${GRPC_DIR}/lib/upb_*.lib")
file(GLOB UTF8_LIBS "${GRPC_DIR}/lib/utf8_*.lib")
file(GLOB ADDR_SORT_LIB "${GRPC_DIR}/lib/address_sorting.lib")
file(GLOB CARES_LIB "${GRPC_DIR}/lib/cares.lib")

# 5. Proto 生成
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/message.proto")
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
         -I "${CMAKE_CURRENT_SOURCE_DIR}"
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC files from ${PROTO_FILE}"
)

# 6. 源文件定义
file(GLOB SERVER_SOURCES "*.cpp")
set(SHARED_SOURCES
    ../GateServer/ConfigMgr.cpp
    ../GateServer/RedisMgr.cpp
    ../GateServer/MysqlMgr.cpp
    ../GateServer/MysqlDao.cpp
)

add_executable(StatusServer ${SERVER_SOURCES} ${SHARED_SOURCES} ${PROTO_SRCS})

if (MSVC)
    add_compile_options("/utf-8")
endif()

target_compile_definitions(StatusServer PRIVATE 
    BOOST_ERROR_CODE_HEADER_ONLY
    WIN32_LEAN_AND_MEAN
)

target_link_libraries(StatusServer PRIVATE 
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

target_sources(StatusServer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")

add_custom_target(CopyStatusServerConfig ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        "$<TARGET_FILE_DIR:StatusServer>/config.ini"
    COMMENT "Updating StatusServer config.ini..."
)

add_dependencies(CopyStatusServerConfig StatusServer)