# 定义源代码 (因为文件都在当前目录，直接写名字即可)
set(SERVER_SOURCES
    main.cpp
    CServer.cpp
    HttpConnection.cpp
    LogicSystem.cpp
)

add_executable(GateServer ${SERVER_SOURCES})

# 宏定义 (Header-Only 模式)
target_compile_definitions(GateServer PRIVATE 
    BOOST_ERROR_CODE_HEADER_ONLY
    WIN32_LEAN_AND_MEAN
)

# 链接库 (链接根目录定义好的库)
target_link_libraries(GateServer PRIVATE 
    Boost::system 
    jsoncpp_lib
    ws2_32
    mswsock
)

# 包含路径
target_include_directories(GateServer PRIVATE 
    ${Boost_INC_DIR}            # Boost 头文件
    ${CMAKE_CURRENT_SOURCE_DIR} # 当前目录 (确保能找到 const.h, Singleton.h)
)