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
    src/TimerBtn.cpp
    src/TimerBtn.h
    src/ClickedLabel.cpp
    src/ClickedLabel.h
    src/ResetDialog.cpp
    src/ResetDialog.h
    resources.qrc
)

qt_add_executable(MemoChatClient
    MANUAL_FINALIZATION
    ${CLIENT_SOURCES}
)

target_sources(MemoChatClient PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")

add_custom_target(CopyClientConfig ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        "$<TARGET_FILE_DIR:MemoChatClient>/config.ini"
    COMMENT "Updating Client config.ini..."
)

# 【关键修改】让复制任务等待 MemoChatClient 编译完成
add_dependencies(CopyClientConfig MemoChatClient)

target_link_libraries(MemoChatClient PRIVATE Qt6::Widgets Qt6::Network)

if(WIN32)
    set_target_properties(MemoChatClient PROPERTIES WIN32_EXECUTABLE ON)
endif()