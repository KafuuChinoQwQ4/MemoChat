cmake_minimum_required(VERSION 3.20)

add_executable(GateServerHttp1.1
    main.cpp
    H1Listener.cpp
    H1Connection.cpp
    H1LogicSystem.cpp
    H1JsonSupport.cpp
    H1WorkerPool.cpp
    H1Globals.cpp
    H1AuthRoutes.cpp
    H1MomentsRoutes.cpp
    H1MediaService.cpp
    H1R18Routes.cpp
)

set_target_properties(GateServerHttp1.1 PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServerHttp1.1)

target_link_libraries(GateServerHttp1.1 PRIVATE
    GateServerCore
    OpenSSL::SSL OpenSSL::Crypto
)

target_include_directories(GateServerHttp1.1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServerCore
)

if(WIN32)
    # Environment variables: WINSDK_LIB_UM, WINSDK_LIB_UCRT, VC_TOOLS_LIB
    target_link_libraries(GateServerHttp1.1 PRIVATE
        "$ENV{WINSDK_LIB_UM}/bcrypt.lib")
    target_link_options(GateServerHttp1.1 PRIVATE
        "/LIBPATH:$ENV{WINSDK_LIB_UM}"
        "/LIBPATH:$ENV{WINSDK_LIB_UCRT}"
        "/LIBPATH:$ENV{VC_TOOLS_LIB}")
endif()

add_executable(R18PluginHost
    R18PluginHost.cpp
)

set_target_properties(R18PluginHost PROPERTIES FOLDER "Server")
target_compile_features(R18PluginHost PRIVATE cxx_std_23)
set_target_properties(R18PluginHost PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_include_directories(R18PluginHost PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT WIN32)
    target_link_libraries(R18PluginHost PRIVATE dl)
endif()

add_library(R18MockSourcePlugin SHARED
    R18MockSourcePlugin.cpp
)

set_target_properties(R18MockSourcePlugin PROPERTIES FOLDER "Server/Plugins")
target_compile_features(R18MockSourcePlugin PRIVATE cxx_std_23)
set_target_properties(R18MockSourcePlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/r18-plugins"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/r18-plugins"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/r18-plugins"
)

target_include_directories(R18MockSourcePlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
