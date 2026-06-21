# runtime/ 目录树

> 测试服务端公共运行时基础设施。当前覆盖 asioexec/stdexec 协程地基(P0 spike):验证 exec::asio::use_sender + exec::task 协程在本项目 gcc-16 工具链下可编译可运行,作为传输层回调换协程改造的前置验证。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `test_asioexec_spike.cpp` | 验证 use_sender→sender 桥接、task co_await async_read/write/wait、顶层协程启动、error_code→system_error 错误语义 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
