# 定义服务端可执行文件
add_executable(GateServer main.cpp)

# 链接库 (Boost 和 JsonCpp 已经在根目录准备好了，直接用)
target_link_libraries(GateServer PRIVATE 
    Boost::boost
    Boost::system
    jsoncpp_lib
)

# 包含头文件路径
target_include_directories(GateServer PRIVATE ${Boost_INCLUDE_DIRS})