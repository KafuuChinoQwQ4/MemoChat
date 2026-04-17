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
)

set_target_properties(GateServerHttp1.1 PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServerHttp1.1)

target_link_libraries(GateServerHttp1.1 PRIVATE
    GateServerCore
    OpenSSL::SSL OpenSSL::Crypto
    Boost::system
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
