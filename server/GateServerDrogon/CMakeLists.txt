cmake_minimum_required(VERSION 3.16)

project(GateServerDrogon LANGUAGES CXX)

# CRITICAL: Pre-define NOMINMAX and use Windows SDK byte workaround BEFORE any drogon/windows headers
# This must be done via compiler definition to ensure it takes effect across all includes
add_definitions(-DNOMINMAX)

add_executable(GateServerDrogon
    main.cpp
    GateServerDrogon.cpp
    GateServerDrogon.h
    DrogonAppCfg.h
    DrogonAppCfg.cpp
    DrogonRoutes.h
    DrogonRoutes.cpp
    DrogonHttpHandlers.h
    DrogonHttpHandlers.cpp
    DrogonAuthHandlers.h
    DrogonAuthHandlers.cpp
    DrogonProfileHandlers.h
    DrogonProfileHandlers.cpp
    DrogonCallHandlers.h
    DrogonCallHandlers.cpp
    DrogonMediaHandlers.h
    DrogonMediaHandlers.cpp
    config.ini
    StdPrefix.h
)

set_target_properties(GateServerDrogon PROPERTIES FOLDER "Server")
memochat_configure_server_target(GateServerDrogon)

# Add GateServer include path for Singleton.h and other shared headers
target_include_directories(GateServerDrogon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../GateServer
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# Drogon-specific configuration
target_compile_features(GateServerDrogon PRIVATE cxx_std_20)
set_target_properties(GateServerDrogon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Link PostgreSQL if available
target_link_libraries(GateServerDrogon PRIVATE ${MEMOCHAT_PQXX_TARGET})

# Link Kafka if enabled
if(MEMOCHAT_ENABLE_KAFKA)
    target_link_libraries(GateServerDrogon PRIVATE ${MEMOCHAT_CPPKAFKA_TARGET})
endif()

# Link RabbitMQ if enabled
if(MEMOCHAT_ENABLE_RABBITMQ)
    target_link_libraries(GateServerDrogon PRIVATE ${MEMOCHAT_RABBITMQ_TARGET})
endif()

# Windows-specific libraries
if(WIN32)
    target_link_libraries(GateServerDrogon PRIVATE bcrypt)
endif()

# Force-include StdPrefix.h to fix byte ambiguity BEFORE any system headers
if(MSVC)
    target_compile_options(GateServerDrogon PRIVATE /FI"${CMAKE_CURRENT_SOURCE_DIR}/StdPrefix.h")
endif()
