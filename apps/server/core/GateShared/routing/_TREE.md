# routing/ 目录树

> Gate 的统一路由抽象层：定义请求/响应、路由模块接口、路由注册表与传输适配契约，使各传输层（H1/H2/H3）共享同一套路由。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateRequest.h` | 统一请求抽象声明。 |
| `GateResponse.h` | 统一响应抽象声明。 |
| `RouteModule.h` | 路由模块接口声明。 |
| `RouteRegistry.cpp` | 路由注册表实现（前缀路由匹配/分发）。 |
| `RouteRegistry.h` | 路由注册表接口声明。 |
| `RouteSchema.h` | 路由 request/response DTO 的只读 schema 与 snapshot helper。 |
| `RouteSchemaCatalog.h` | 跨模块路由 schema 聚合目录：收集各模块 schema、渲染文本/JSON 元数据文档并检测重复 method+path。 |
| `TransportAdapterContract.md` | 传输层适配契约说明文档。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
