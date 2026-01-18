# ==========================================
# GateServer - CMake 配置 (集成 gRPC)
# ==========================================

# 1. 定义 gRPC 的安装路径 (Day 6 编译的结果)
set(GRPC_DIR "D:/cppsoft/grpc/install")

# 2. 查找并设置头文件路径
include_directories(
    ${Boost_INC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR} # 生成的 .pb.h 文件会在这里
    "${GRPC_DIR}/include"       # gRPC 头文件
)

# 3. 查找并设置库文件路径
link_directories(
    "${GRPC_DIR}/lib"
)

# 4. 自动生成 gRPC 代码的命令
# 定义 proto 文件位置
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../message.proto") # 放在 Server 根目录方便共享
# 或者如果你的 proto 就在 GateServer 目录下：
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/message.proto") 

# 定义生成工具路径
set(PROTOC "${GRPC_DIR}/bin/protoc.exe")
set(GRPC_CPP_PLUGIN "${GRPC_DIR}/bin/grpc_cpp_plugin.exe")

# 定义生成的源文件名
set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/message.pb.cc" "${CMAKE_CURRENT_BINARY_DIR}/message.grpc.pb.cc")
set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/message.pb.h" "${CMAKE_CURRENT_BINARY_DIR}/message.grpc.pb.h")

# 添加自定义命令：每次构建前自动生成 .pb.cc 和 .pb.h
add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND ${PROTOC}
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         -I "${CMAKE_CURRENT_SOURCE_DIR}" # proto 文件所在目录
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC files from message.proto"
)

# 5. 定义源代码列表 (加入新写的类和生成的 proto 代码)
set(SERVER_SOURCES
    main.cpp
    CServer.cpp
    HttpConnection.cpp
    LogicSystem.cpp
    VerifyGrpcClient.cpp  # 新增：gRPC 客户端类
    ConfigMgr.cpp         # 新增：配置管理类
    ${PROTO_SRCS}         # 新增：生成的 gRPC 代码
)

# 6. 生成可执行文件
add_executable(GateServer ${SERVER_SOURCES})

# 7. 编译选项
target_compile_definitions(GateServer PRIVATE 
    BOOST_ERROR_CODE_HEADER_ONLY
    WIN32_LEAN_AND_MEAN
)

# 8. 链接库 (顺序很重要，gRPC 依赖较多)
target_link_libraries(GateServer PRIVATE 
    # Boost & Json
    Boost::system 
    jsoncpp_lib
    
    # gRPC & Protobuf
    libprotobuf 
    gpr 
    grpc 
    grpc++ 
    # 如果 Day 6 编译生成了 reflection 库，可以加上: grpc++_reflection
    
    # gRPC 依赖的 SSL 库 (Day 6 编译生成的)
    ssl 
    crypto 
    
    # gRPC 依赖的 Windows 系统库
    ws2_32 
    crypt32
    mswsock
)