cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(transport_endpoint_policy_gtest
    TransportEndpointPolicyTest.cpp
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/shared/gateway/TransportEndpointPolicy.cpp
)
set_target_properties(transport_endpoint_policy_gtest PROPERTIES
    CXX_STANDARD ${MEMOCHAT_QT_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(transport_endpoint_policy_gtest PRIVATE
    GTest_TestLib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_include_directories(transport_endpoint_policy_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/shared/gateway
    ${CMAKE_SOURCE_DIR}/apps/client/desktop/MemoChat-qml/core/common
)

add_dependencies(gtest_tests transport_endpoint_policy_gtest)

include(GoogleTest)
gtest_discover_tests(transport_endpoint_policy_gtest TEST_DISCOVERY_MODE POST_BUILD)
