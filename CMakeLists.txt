cmake_minimum_required(VERSION 3.16)
project(MemoChat VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(Boost_INC_DIR "D:/cppsoft/boost_1_90_0")
set(Boost_LIB_DIR "D:/cppsoft/boost_1_90_0/lib64-msvc-14.3")

add_library(Boost::boost INTERFACE IMPORTED)
set_target_properties(Boost::boost PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Boost_INC_DIR}")

add_library(Boost::system INTERFACE IMPORTED)
set_target_properties(Boost::system PROPERTIES 
    INTERFACE_INCLUDE_DIRECTORIES "${Boost_INC_DIR}"
    INTERFACE_COMPILE_DEFINITIONS "BOOST_ERROR_CODE_HEADER_ONLY"
)

include(FetchContent)
FetchContent_Declare(jsoncpp GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git GIT_TAG 1.9.5)
FetchContent_MakeAvailable(jsoncpp)

add_subdirectory(Client)
add_subdirectory(Server)