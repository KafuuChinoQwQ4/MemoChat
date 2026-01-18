# ==========================================
# GateServer - CMake 配置 (最终通过版)
# ==========================================

# 1. 定义 gRPC 的安装路径
set(GRPC_DIR "D:/cppsoft/grpc/install")

# 2. 头文件路径 (包含 JsonCpp)
include_directories(
    ${Boost_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    "${GRPC_DIR}/include"
    "${CMAKE_BINARY_DIR}/_deps/jsoncpp-src/include"
)

# 3. 库文件路径
link_directories(
    "${GRPC_DIR}/lib"
    "D:/cppsoft/boost_1_90_0/lib64-msvc-14.3"
)

# 4. === 自动查找所有依赖库 ===
# Abseil
file(GLOB ABSL_LIBS "${GRPC_DIR}/lib/absl_*.lib")
# UPB (Micro Protobuf)
file(GLOB UPB_LIBS "${GRPC_DIR}/lib/upb_*.lib")
# UTF8 Range
file(GLOB UTF8_LIBS "${GRPC_DIR}/lib/utf8_*.lib")
# Address Sorting
file(GLOB ADDR_SORT_LIB "${GRPC_DIR}/lib/address_sorting.lib")
# === [修复] C-Ares (DNS解析库) ===
file(GLOB CARES_LIB "${GRPC_DIR}/lib/cares.lib")

# 5. 自动生成 gRPC 代码
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
    COMMENT "Generating gRPC files from message.proto"
)

# 6. 源代码列表
set(SERVER_SOURCES
    main.cpp
    CServer.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    VerifyGrpcClient.cpp
    ConfigMgr.cpp
    AsioIOServicePool.cpp
    ${PROTO_SRCS}
)

# 7. 生成可执行文件
add_executable(GateServer ${SERVER_SOURCES})

# 8. 编译选项 (UTF-8)
if (MSVC)
    add_compile_options("/utf-8")
endif()

target_compile_definitions(GateServer PRIVATE 
    BOOST_ERROR_CODE_HEADER_ONLY
    WIN32_LEAN_AND_MEAN
)

# 9. 链接库
target_link_libraries(GateServer PRIVATE 
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
    ${CARES_LIB}      # <--- 加上这一行，解决 ares_xxx 报错
    
    re2
    ssl 
    crypto 
    zlib
    
    ws2_32 
    crypt32
    mswsock
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
    "${CMAKE_CURRENT_BINARY_DIR}/config.ini"
    COPYONLY
)

add_custom_command(TARGET GateServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        "$<TARGET_FILE_DIR:GateServer>/config.ini"
    COMMENT "Copying config.ini to output directory"
)