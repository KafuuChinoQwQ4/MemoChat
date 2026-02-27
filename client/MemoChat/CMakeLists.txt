cmake_minimum_required(VERSION 3.14)
project(MemoChat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# For MSVC, add specific compiler flags
if(MSVC)
    add_compile_options(/wd4819 /utf-8)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

set(SOURCES
    BubbleFrame.cpp
    ChatItemBase.cpp
    ChatView.cpp
    MessageTextEdit.cpp
    PictureBubble.cpp
    TextBubble.cpp
    adduseritem.cpp
    applyfriend.cpp
    applyfrienditem.cpp
    applyfriendlist.cpp
    applyfriendpage.cpp
    authenfriend.cpp
    chatdialog.cpp
    chatpage.cpp
    chatuserlist.cpp
    chatuserwid.cpp
    clickedbtn.cpp
    clickedlabel.cpp
    clickedoncelabel.cpp
    contactuserlist.cpp
    conuseritem.cpp
    customizeedit.cpp
    customizetextedit.cpp
    findfaildlg.cpp
    findsuccessdlg.cpp
    friendinfopage.cpp
    friendlabel.cpp
    global.cpp
    grouptipitem.cpp
    httpmgr.cpp
    imagecropperlabel.cpp
    invaliditem.cpp
    lineitem.cpp
    listitembase.cpp
    loadingdlg.cpp
    logindialog.cpp
    main.cpp
    mainwindow.cpp
    registerdialog.cpp
    resetdialog.cpp
    searchlist.cpp
    statelabel.cpp
    statewidget.cpp
    tcpmgr.cpp
    timerbtn.cpp
    userdata.cpp
    userinfopage.cpp
    usermgr.cpp
)

set(HEADERS
    BubbleFrame.h
    ChatItemBase.h
    ChatView.h
    MessageTextEdit.h
    PictureBubble.h
    TextBubble.h
    adduseritem.h
    applyfriend.h
    applyfrienditem.h
    applyfriendlist.h
    applyfriendpage.h
    authenfriend.h
    chatdialog.h
    chatpage.h
    chatuserlist.h
    chatuserwid.h
    clickedbtn.h
    clickedlabel.h
    clickedoncelabel.h
    contactuserlist.h
    conuseritem.h
    customizeedit.h
    customizetextedit.h
    findfaildlg.h
    findsuccessdlg.h
    friendinfopage.h
    friendlabel.h
    global.h
    grouptipitem.h
    httpmgr.h
    imagecropperdialog.h
    imagecropperlabel.h
    invaliditem.h
    lineitem.h
    listitembase.h
    loadingdlg.h
    logindialog.h
    mainwindow.h
    registerdialog.h
    resetdialog.h
    searchlist.h
    singleton.h
    statelabel.h
    statewidget.h
    tcpmgr.h
    timerbtn.h
    userdata.h
    userinfopage.h
    usermgr.h
)

set(FORMS
    adduseritem.ui
    applyfriend.ui
    applyfrienditem.ui
    applyfriendpage.ui
    authenfriend.ui
    chatdialog.ui
    chatpage.ui
    chatuserwid.ui
    conuseritem.ui
    findfaildlg.ui
    findsuccessdlg.ui
    friendinfopage.ui
    friendlabel.ui
    grouptipitem.ui
    lineitem.ui
    loadingdlg.ui
    logindialog.ui
    mainwindow.ui
    registerdialog.ui
    resetdialog.ui
    userinfopage.ui
)

set(RESOURCES
    rc.qrc
)

add_executable(MemoChat WIN32 MACOSX_BUNDLE ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES})

target_include_directories(MemoChat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MemoChat PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

# Copy config.ini to the output directory
add_custom_command(TARGET MemoChat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
    $<TARGET_FILE_DIR:MemoChat>/config.ini
    COMMENT "Copying config.ini"
)

# Copy the static directory to the output directory
add_custom_command(TARGET MemoChat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/static
    $<TARGET_FILE_DIR:MemoChat>/static
    COMMENT "Copying static directory"
)

# Copy the res directory to the output directory
add_custom_command(TARGET MemoChat POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/res
    $<TARGET_FILE_DIR:MemoChat>/res
    COMMENT "Copying res directory"
)
