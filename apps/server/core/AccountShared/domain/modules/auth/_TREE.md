# auth/ 目录树

> 认证路由模块，将登录、refresh、注册和重置密码等认证类请求映射到认证领域服务。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 认证路由 schema 的 C++ module 辅助代码 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthRouteModule.cpp` | 认证路由模块实现，注册登录与 refresh token 路由 |
| `AuthRouteModule.h` | 认证路由模块声明 |
| `AuthRouteSchemas.cpp` | 认证稳定 JSON 路由的只读 schema descriptor 旁列表，包含登录与 refresh 响应契约 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
