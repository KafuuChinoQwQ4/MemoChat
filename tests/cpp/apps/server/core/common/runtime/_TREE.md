# runtime/ 目录树

> 测试服务端公共运行时基础设施。当前覆盖 asioexec/stdexec 协程地基、生产线程亲和验证，以及不进 CMake 的网络写法对比示例。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `EtcdConfigAlgorithmsConsumer.cpp` | 测试侧导入 Etcd 配置算法模块并暴露 endpoint、响应和 watch/key helper 给 GTest 断言。 |
| `IniConfigTest.cpp` | 验证公共运行时 INI 环境变量 key 生成和 sanitize 行为，覆盖 C++ modules 导入后的真实 memochat_runtime 路径。 |
| `IoContextPoolAlgorithmsConsumer.cpp` | 测试侧导入 IO 上下文池算法模块并暴露大小、索引和停止 helper 给 GTest 断言。 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `stdexec_network_json_comparison_demo.cpp` | 不进 CMake 的阅读示例，对比认证重试、命令循环和分支处理在 stdexec 协程与传统 Asio 拆函数回调下的写法 |
| `test_asioexec_prodpath.cpp` | 验证 asioexec/stdexec 生产路径的 io_context 线程亲和、取消语义和读写循环形态 |
| `test_asioexec_spike.cpp` | 验证 use_sender→sender 桥接、task co_await async_read/write/wait、顶层协程启动、error_code→system_error 错误语义 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
