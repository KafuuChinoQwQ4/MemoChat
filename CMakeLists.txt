cmake_minimum_required(VERSION 3.16)
project(MemoChat VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === [修改开始] Boost 路径配置 ===
# 关键修复：变量名必须是 Boost_ROOT (区分大小写)
set(Boost_ROOT "D:/cppsoft/boost_1_90_0")
set(Boost_INCLUDE_DIR "D:/cppsoft/boost_1_90_0")
# 关键修复：指定库目录变量名为 Boost_LIBRARY_DIR
set(Boost_LIBRARY_DIR "D:/cppsoft/boost_1_90_0/lib64-msvc-14.3")

# 忽略系统路径，强制使用我们指定的路径
set(Boost_NO_SYSTEM_PATHS ON) 

# 显式查找 Boost 组件
find_package(Boost REQUIRED COMPONENTS system thread)
# === [修改结束] ===

include(FetchContent)
FetchContent_Declare(jsoncpp GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git GIT_TAG 1.9.5)
FetchContent_MakeAvailable(jsoncpp)

add_subdirectory(Client)
add_subdirectory(Server)