find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CLIENT_SOURCES
    main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/LoginDialog.cpp
    src/LoginDialog.h
    src/RegisterDialog.cpp
    src/RegisterDialog.h
    src/global.cpp
    src/global.h
    src/HttpMgr.cpp
    src/HttpMgr.h
    src/Singleton.h
    resources.qrc
)

qt_add_executable(MemoChatClient
    MANUAL_FINALIZATION
    ${CLIENT_SOURCES}
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
    "${CMAKE_CURRENT_BINARY_DIR}/config.ini"
    COPYONLY
)

add_custom_command(TARGET MemoChatClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        "$<TARGET_FILE_DIR:MemoChatClient>/config.ini"
)

target_link_libraries(MemoChatClient PRIVATE Qt6::Widgets Qt6::Network)

if(WIN32)
    set_target_properties(MemoChatClient PROPERTIES WIN32_EXECUTABLE ON)
endif()