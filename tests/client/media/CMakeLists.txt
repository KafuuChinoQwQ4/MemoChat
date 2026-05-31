cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui)

add_executable(image_cropper_geometry_gtest
    ImageCropperGeometryTest.cpp
    ../../../apps/client/desktop/MemoChat-qml/core/media/ImageCropperGeometry.cpp
    ../../../apps/client/desktop/MemoChat-qml/core/media/ImageCropperCursorPolicy.cpp
    ../../../apps/client/desktop/MemoChat-qml/core/media/ImageCropperHitTest.cpp
    ../../../apps/client/desktop/MemoChat-qml/core/media/ImageCropperInteractionState.cpp
)

target_link_libraries(image_cropper_geometry_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
)

target_include_directories(image_cropper_geometry_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/media
)

add_dependencies(gtest_tests image_cropper_geometry_gtest)

include(GoogleTest)
gtest_discover_tests(image_cropper_geometry_gtest TEST_DISCOVERY_MODE POST_BUILD)
