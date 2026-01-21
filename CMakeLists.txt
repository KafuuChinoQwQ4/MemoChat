cmake_minimum_required(VERSION 3.16)
project(MemoChat VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === [关键修复 1] 配置 Boost 路径和库目录 ===
set(BOOST_ROOT_DIR "D:/cppsoft/boost_1_90_0")
set(BOOST_LIB_DIR "D:/cppsoft/boost_1_90_0/lib64-msvc-14.3")

# 让链接器能找到 .lib 文件 (这对 Windows MSVC Auto-Linking 至关重要)
link_directories(${BOOST_LIB_DIR})

# 定义 Boost::boost (头文件)
add_library(Boost::boost INTERFACE IMPORTED)
set_target_properties(Boost::boost PROPERTIES 
    INTERFACE_INCLUDE_DIRECTORIES "${BOOST_ROOT_DIR}"
)

# 定义 Boost::system
add_library(Boost::system INTERFACE IMPORTED)
set_target_properties(Boost::system PROPERTIES 
    INTERFACE_INCLUDE_DIRECTORIES "${BOOST_ROOT_DIR}"
)

# === [关键修复 2] 补全 Boost::thread 定义 ===
add_library(Boost::thread INTERFACE IMPORTED)
set_target_properties(Boost::thread PROPERTIES 
    INTERFACE_INCLUDE_DIRECTORIES "${BOOST_ROOT_DIR}"
)

include(FetchContent)
FetchContent_Declare(jsoncpp GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git GIT_TAG 1.9.5)
FetchContent_MakeAvailable(jsoncpp)

add_subdirectory(Client)
add_subdirectory(Server)