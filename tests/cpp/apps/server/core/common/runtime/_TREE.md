# runtime/ 目录树

> 测试服务端公共运行时基础设施，覆盖 INI/Etcd 配置和 IO 上下文池算法。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `EtcdConfigAlgorithmsConsumer.cpp` | 测试侧导入 Etcd 配置算法模块并暴露 endpoint、响应和 watch/key helper 给 GTest 断言。 |
| `ExplicitThreadTest.cpp` | 验证无异常线程适配器的启动、重复启动拒绝、等待和硬件并发数。 |
| `IniConfigTest.cpp` | 验证公共运行时 INI 环境变量 key 生成和 sanitize 行为，覆盖 C++ modules 导入后的真实 memochat_runtime 路径。 |
| `IoContextPoolAlgorithmsConsumer.cpp` | 测试侧导入 IO 上下文池算法模块并暴露大小、索引和停止 helper 给 GTest 断言。 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
