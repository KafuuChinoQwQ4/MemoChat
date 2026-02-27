cmake_minimum_required(VERSION 3.16)

project(MemoChatServer LANGUAGES CXX)

if(MSVC)
    # Match vcpkg x64-windows-static triplet (MT/MTd) to avoid runtime mismatch at link time.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(unofficial-libmysql CONFIG REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)

if(NOT TARGET protobuf::protoc)
    message(FATAL_ERROR "protobuf::protoc target not found. Install protobuf host tools.")
endif()

if(NOT TARGET gRPC::grpc_cpp_plugin)
    message(FATAL_ERROR "gRPC::grpc_cpp_plugin target not found. Install grpc[codegen].")
endif()

add_library(memochat_server_deps INTERFACE)
target_compile_features(memochat_server_deps INTERFACE cxx_std_17)
target_link_libraries(memochat_server_deps INTERFACE
    Boost::filesystem
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    hiredis::hiredis
    JsonCpp::JsonCpp
    unofficial::libmysql::libmysql
    unofficial::mysql-connector-cpp::connector-jdbc
    ws2_32
    crypt32
    advapi32
    iphlpapi
)

function(memochat_add_proto_sources target_name proto_file)
    get_filename_component(proto_name "${proto_file}" NAME_WE)
    set(proto_source "${CMAKE_CURRENT_SOURCE_DIR}/${proto_file}")
    set(proto_cc "${CMAKE_CURRENT_SOURCE_DIR}/${proto_name}.pb.cc")
    set(proto_h "${CMAKE_CURRENT_SOURCE_DIR}/${proto_name}.pb.h")
    set(grpc_cc "${CMAKE_CURRENT_SOURCE_DIR}/${proto_name}.grpc.pb.cc")
    set(grpc_h "${CMAKE_CURRENT_SOURCE_DIR}/${proto_name}.grpc.pb.h")

    add_custom_command(
        OUTPUT
            "${proto_cc}"
            "${proto_h}"
            "${grpc_cc}"
            "${grpc_h}"
        COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR} ${proto_source}
        COMMAND $<TARGET_FILE:protobuf::protoc> --proto_path=${CMAKE_CURRENT_SOURCE_DIR} --grpc_out=${CMAKE_CURRENT_SOURCE_DIR} --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin> ${proto_source}
        DEPENDS
            "${proto_source}"
            protobuf::protoc
            gRPC::grpc_cpp_plugin
        VERBATIM
    )

    target_sources(${target_name} PRIVATE
        "${proto_cc}"
        "${proto_h}"
        "${grpc_cc}"
        "${grpc_h}"
    )
endfunction()

function(memochat_configure_server_target target_name)
    target_link_libraries(${target_name} PRIVATE memochat_server_deps)
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
            "$<TARGET_FILE_DIR:${target_name}>/config.ini"
        )
    endif()

    file(GLOB local_dlls "${CMAKE_CURRENT_SOURCE_DIR}/*.dll")
    foreach(local_dll IN LISTS local_dlls)
        add_custom_command(
            TARGET ${target_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${local_dll}"
            "$<TARGET_FILE_DIR:${target_name}>"
        )
    endforeach()
endfunction()

add_subdirectory(ChatServer)
add_subdirectory(ChatServer2)
add_subdirectory(GateServer)
add_subdirectory(StatusServer)
add_subdirectory(VarifyServer)
