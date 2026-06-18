# legacy_standalone/ 目录树

> 退役的 HTTP/1 独立网关进程：自带连接/监听/逻辑分发与各业务路由（Auth/Media/Moments/R18），作为历史参考保留。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 独立 H1 网关构建目标定义。 |
| `H1AuthRoutes.cpp` | H1 鉴权路由实现。 |
| `H1AuthRoutes.h` | H1 鉴权路由声明。 |
| `H1Connection.cpp` | H1 连接处理实现。 |
| `H1Connection.h` | H1 连接接口声明。 |
| `H1Globals.cpp` | H1 全局对象实现。 |
| `H1Globals.h` | H1 全局对象声明。 |
| `H1JsonSupport.cpp` | H1 JSON 支持实现。 |
| `H1JsonSupport.h` | H1 JSON 支持声明。 |
| `H1Listener.cpp` | H1 监听器实现。 |
| `H1Listener.h` | H1 监听器声明。 |
| `H1LogicSystem.cpp` | H1 逻辑分发系统实现。 |
| `H1LogicSystem.h` | H1 逻辑分发系统声明。 |
| `H1MediaService.cpp` | H1 媒体服务实现。 |
| `H1MediaService.h` | H1 媒体服务声明。 |
| `H1MomentsRoutes.cpp` | H1 朋友圈路由实现。 |
| `H1MomentsRoutes.h` | H1 朋友圈路由声明。 |
| `H1R18Routes.cpp` | H1 R18 路由实现。 |
| `H1R18Routes.h` | H1 R18 路由声明。 |
| `H1WorkerPool.cpp` | H1 工作线程池实现。 |
| `H1WorkerPool.h` | H1 工作线程池声明。 |
| `README.md` | 退役 H1 standalone 说明文档。 |
| `config.ini` | 运行时配置。 |
| `main.cpp` | 独立 H1 网关进程入口。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
