# runtime/ 目录树

> Gate 的运行时基础组件：异步 IO 线程池、全局对象与单例工具。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AsioIOServicePool.cpp` | Asio IO 服务线程池实现。 |
| `AsioIOServicePool.h` | IO 线程池接口声明。 |
| `GateGlobals.cpp` | Gate 全局对象/状态定义实现。 |
| `GateGlobals.h` | Gate 全局对象声明。 |
| `GateReadinessProbe.hpp` | 传输/管理器无关的启动就绪探针契约：各 infra slice 提供探针工厂，域服务 bootstrap 只依赖该契约而不引用具体 Mgr。 |
| `Singleton.h` | 通用单例模板。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
