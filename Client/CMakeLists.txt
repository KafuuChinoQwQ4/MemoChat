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

target_link_libraries(MemoChatClient PRIVATE Qt6::Widgets Qt6::Network)

if(WIN32)
    set_target_properties(MemoChatClient PROPERTIES WIN32_EXECUTABLE ON)
endif()