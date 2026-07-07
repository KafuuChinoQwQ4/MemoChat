# support/ 目录树

> Gate 的跨模块支撑组件，含 Bearer access token 解析和共享 JWT/Redis 用户 token 校验器。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | support 层 C++ modules 接口单元。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `BearerAccessAuth.cpp` | 从 GateRequest 的 Authorization 头解析 Bearer access token 并解析当前用户 uid |
| `BearerAccessAuth.hpp` | Bearer access token 解析与 uid 解析接口声明 |
| `GateRouteModules.hpp` | 路由模块声明聚合（support 层视图）。 |
| `UserTokenValidator.cpp` | 用户 access token 校验实现，验证 JWT 后再检查 Redis 当前 token 绑定。 |
| `UserTokenValidator.hpp` | 用户 access token 校验接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
