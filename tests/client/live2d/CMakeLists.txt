cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(CMAKE_AUTOMOC ON)

add_executable(live2d_asset_gtest
    main.cpp
    Live2DAssetTest.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAsset.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetActionFiles.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetModelFile.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetOptionalFiles.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetPaths.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetRequiredFiles.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetValidation.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetVoiceFiles.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DModelAssetParser.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DMotionCatalog.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DRenderPathResolver.cpp
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAsset.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetActionFiles.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetModelFile.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetOptionalFiles.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetPaths.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetRequiredFiles.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetValidation.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DAssetVoiceFiles.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DModelAssetParser.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DMotionCatalog.h
    ../../../apps/client/desktop/MemoChat-qml/live2d/Live2DRenderPathResolver.h
)

target_link_libraries(live2d_asset_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(live2d_asset_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d
)

target_compile_definitions(live2d_asset_gtest PRIVATE
    LIVE2D_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/live2d"
)

add_dependencies(gtest_tests live2d_asset_gtest)

include(GoogleTest)
gtest_discover_tests(live2d_asset_gtest TEST_DISCOVERY_MODE POST_BUILD)
