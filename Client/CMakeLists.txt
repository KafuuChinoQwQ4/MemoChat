find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
    add_compile_options(/wd4819 /utf-8)
endif()

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
    src/TcpMgr.cpp
    src/TcpMgr.h
    src/UserMgr.cpp
    src/UserMgr.h
    src/ClickedBtn.cpp
    src/ClickedBtn.h
    src/ChatDialog.cpp
    src/ChatDialog.h
    src/CustomizeEdit.cpp
    src/CustomizeEdit.h
    src/ChatUserWid.cpp
    src/ChatUserWid.h
    src/ChatUserList.cpp
    src/ChatUserList.h
    src/LoadingDlg.cpp
    src/LoadingDlg.h
    src/ChatPage.cpp
    src/ChatPage.h
    src/ChatView.cpp
    src/ChatView.h
    src/ChatItemBase.cpp
    src/ChatItemBase.h
    src/BubbleFrame.cpp
    src/BubbleFrame.h
    src/StateWidget.cpp
    src/StateWidget.h
    src/SearchList.cpp
    src/SearchList.h
    src/AddUserItem.cpp
    src/AddUserItem.h
    src/ListItemBase.cpp
    src/ListItemBase.h
    src/FindSuccessDlg.cpp
    src/FindSuccessDlg.h
    src/ClickedOnceLabel.cpp
    src/ClickedOnceLabel.h
    src/FriendLabel.cpp
    src/FriendLabel.h
    src/ApplyFriend.cpp
    src/ApplyFriend.h
    src/GroupTipItem.cpp 
    src/GroupTipItem.h
    src/ConUserItem.cpp 
    src/ConUserItem.h
    src/ContactUserList.cpp 
    src/ContactUserList.h
    src/ApplyFriendItem.cpp 
    src/ApplyFriendItem.h
    src/ApplyFriendList.cpp 
    src/ApplyFriendList.h
    src/ApplyFriendPage.cpp 
    src/ApplyFriendPage.h
    src/userdata.h
    src/AuthenFriend.cpp
    src/AuthenFriend.h
    src/FindFailDlg.cpp
    src/FindFailDlg.h
)

qt_add_executable(MemoChatClient
    MANUAL_FINALIZATION
    ${CLIENT_SOURCES}
)

target_sources(MemoChatClient PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")

# ... 前面的代码不变 ...

add_custom_target(CopyClientConfig ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.ini"
        "$<TARGET_FILE_DIR:MemoChatClient>/config.ini"
    COMMENT "Updating Client config.ini..."
)

# [正确写法] 让复制任务等待 MemoChatClient 编译完成
add_dependencies(CopyClientConfig MemoChatClient)

# ... 后面的代码不变 ... 

target_link_libraries(MemoChatClient PRIVATE Qt6::Widgets Qt6::Network)

if(WIN32)
    set_target_properties(MemoChatClient PROPERTIES WIN32_EXECUTABLE ON)
endif()