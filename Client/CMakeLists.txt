cmake_minimum_required(VERSION 3.16)

project(MemoChat VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt6 组件
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# 自动处理 Qt 元对象系统
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON) # 这一行让 CMake 自动处理 .qrc 文件

# 资源文件
set(RESOURCES resources.qrc)

# 源文件列表
set(PROJECT_SOURCES
    main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/LoginDialog.cpp
    src/LoginDialog.h
    src/RegisterDialog.cpp
    src/RegisterDialog.h
    ${RESOURCES}
)

# 生成可执行文件
qt_add_executable(MemoChat
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# 链接库
target_link_libraries(MemoChat PRIVATE Qt6::Widgets)

# Windows 子系统设置 (发布时去掉黑框，调试时可以注释掉以便看 qDebug)
if(WIN32)
    set_target_properties(MemoChat PROPERTIES WIN32_EXECUTABLE ON)
endif()