cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(CMAKE_AUTOMOC ON)

add_executable(live2d_asset_gtest
    main.cpp
    Live2DAssetTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAsset.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetActionFiles.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetModelFile.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetOptionalFiles.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetPaths.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetRequiredFiles.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetValidation.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetVoiceFiles.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/model/Live2DModelAssetParser.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/model/Live2DMotionCatalog.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/rendering/Live2DRenderPathResolver.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAsset.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetActionFiles.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetModelFile.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetOptionalFiles.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetPaths.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetRequiredFiles.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetValidation.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets/Live2DAssetVoiceFiles.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/model/Live2DModelAssetParser.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/model/Live2DMotionCatalog.h
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/rendering/Live2DRenderPathResolver.h
)

target_link_libraries(live2d_asset_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(live2d_asset_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/assets
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/model
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/live2d/rendering
)

target_compile_definitions(live2d_asset_gtest PRIVATE
    LIVE2D_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/live2d"
)

add_dependencies(gtest_tests live2d_asset_gtest)

include(GoogleTest)
gtest_discover_tests(live2d_asset_gtest TEST_DISCOVERY_MODE POST_BUILD)
