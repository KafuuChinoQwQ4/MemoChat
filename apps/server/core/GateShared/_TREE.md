# GateShared/ 目录树

> Gate 微服务的共享库（原 GateServer framework 改名）：提供各聚焦域网关（Auth/Media/Moments/Call/R18 等）共用的服务器骨架、路由注册、传输层（H1/H2/H3）、核心基础设施与可插拔路由模块/档案。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`adapters/`](adapters/_TREE.md) | 传输层到统一路由契约的适配器。 |
| [`app/`](app/_TREE.md) | 聚焦域网关进程的共享入口骨架。 |
| [`core/`](core/_TREE.md) | 核心基础设施（缓存/客户端/配置/持久化/运行时/支撑）。 |
| [`modules/`](modules/_TREE.md) | 可插拔路由模块（如健康检查）。 |
| [`plugins/`](plugins/_TREE.md) | 外部能力插件（如 R18 源）。 |
| [`profiles/`](profiles/_TREE.md) | 路由档案装配（决定网关启用哪些模块）。 |
| [`routing/`](routing/_TREE.md) | 统一路由抽象：请求/响应、模块与注册表。 |
| [`transports/`](transports/_TREE.md) | HTTP/1、HTTP/2、HTTP/3 传输实现。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | GateShared 库的构建目标定义。 |
| `CServer.cpp` | 基础 TCP/HTTP 服务器实现。 |
| `CServer.h` | 服务器接口声明。 |
| `CertUtil.cpp` | TLS 证书工具实现。 |
| `CertUtil.h` | 证书工具接口声明。 |
| `GateHttpJsonSupport.cpp` | Gate HTTP 的 JSON 解析/构造支持实现。 |
| `GateHttpJsonSupport.h` | JSON 支持接口声明。 |
| `GateRouteModules.h` | Gate 各业务路由模块（Auth/Profile 等）声明聚合。 |
| `GateRouteProfileRegistrar.h` | 路由档案注册器声明。 |
| `GateServerH1Routes.cpp` | Gate 的 HTTP/1 路由装配实现。 |
| `GateWorkerPool.cpp` | 工作线程池实现。 |
| `GateWorkerPool.h` | 工作线程池接口声明。 |
| `HttpConnection.cpp` | HTTP 连接处理实现。 |
| `HttpConnection.h` | HTTP 连接接口声明。 |
| `LogicSystem.cpp` | 业务逻辑分发系统实现。 |
| `LogicSystem.h` | 逻辑分发系统接口声明（单例）。 |
| `PropertySheet.props` | （Windows）构建属性表。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
