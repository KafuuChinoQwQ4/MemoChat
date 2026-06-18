# domain/ 目录树

> AI 网关领域层：定义 AI HTTP 路由、路由模块注册以及对下游 AI 服务的客户端封装。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`modules/`](modules/_TREE.md) | 可插拔的 AI 路由模块实现。 |
| [`profiles/`](profiles/_TREE.md) | AI 网关的路由档案（profile）装配。 |
| [`services/`](services/_TREE.md) | AI 业务服务实现。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AIHttpServiceRoutes.h` | AI HTTP 服务路由声明。 |
| `AIRouteModules.cpp` | AI 路由模块的集中注册实现。 |
| `AIRouteModules.h` | AI 路由模块注册接口声明。 |
| `AIServiceClient.cpp` | 调用下游 AI 服务的客户端实现。 |
| `AIServiceClient.h` | AI 服务客户端接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
