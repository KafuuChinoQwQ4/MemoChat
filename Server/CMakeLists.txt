# 定义源代码列表
set(SERVER_SOURCES
    main.cpp
    CServer.cpp
    HttpConnection.cpp
    LogicSystem.cpp
)

add_executable(GateServer ${SERVER_SOURCES})

# 告诉编译器 System 库是 Header-Only 的 (保持 Day 3 的配置)
target_compile_definitions(GateServer PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)

target_link_libraries(GateServer PRIVATE 
    Boost::system 
    jsoncpp_lib
)

# 引入头文件路径
target_include_directories(GateServer PRIVATE ${Boost_INC_DIR})