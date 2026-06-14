cmake_minimum_required(VERSION 3.20)

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
