cmake_minimum_required(VERSION 3.16)

# P0 地基 spike:验证 exec::asio::use_sender + exec::task<T> 协程在本项目
# gcc-16 + C++23 下可编译可运行。memochat_server_deps 已带入 STDEXEC::asioexec
# 与 boost,直接 link 即可。

add_executable(runtime_gtest_asioexec_spike
    main.cpp
    test_asioexec_spike.cpp
    test_asioexec_prodpath.cpp
)
target_link_libraries(runtime_gtest_asioexec_spike PRIVATE
    GTest_TestLib
    memochat_server_deps
)
target_include_directories(runtime_gtest_asioexec_spike PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests runtime_gtest_asioexec_spike)

add_executable(runtime_ini_config_gtest
    main.cpp
    EtcdConfigAlgorithmsConsumer.cpp
    IniConfigTest.cpp
    IoContextPoolAlgorithmsConsumer.cpp
)
target_link_libraries(runtime_ini_config_gtest PRIVATE
    GTest_TestLib
    memochat_runtime
    memochat_server_deps
)
target_include_directories(runtime_ini_config_gtest PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/server/core/common
)

add_dependencies(gtest_tests runtime_ini_config_gtest)

if(MEMOCHAT_ENABLE_GNU_MODULES)
    memochat_enable_gnu_modules(runtime_ini_config_gtest
        MODULES
            memochat.runtime.etcd_config_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/common/runtime/cxx_modules/EtcdConfig.cppm
            memochat.runtime.io_context_pool_algorithms=${CMAKE_SOURCE_DIR}/apps/server/core/common/runtime/cxx_modules/IoContextPool.cppm
        CONSUMER_SOURCES
            EtcdConfigAlgorithmsConsumer.cpp
            IoContextPoolAlgorithmsConsumer.cpp
    )
endif()

include(GoogleTest)
gtest_discover_tests(runtime_gtest_asioexec_spike TEST_DISCOVERY_MODE POST_BUILD)
gtest_discover_tests(runtime_ini_config_gtest TEST_DISCOVERY_MODE POST_BUILD)
