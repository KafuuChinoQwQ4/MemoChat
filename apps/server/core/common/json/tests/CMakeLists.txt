cmake_minimum_required(VERSION 3.16)

add_executable(json_gtest_glaze_compat_test
    main.cpp
    GlazeCompatTest.cpp
)

target_link_libraries(json_gtest_glaze_compat_test PRIVATE
    GTest_TestLib
    memochat_server_deps
)

target_include_directories(json_gtest_glaze_compat_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(gtest_tests json_gtest_glaze_compat_test)

include(GoogleTest)
gtest_discover_tests(json_gtest_glaze_compat_test TEST_DISCOVERY_MODE POST_BUILD)

