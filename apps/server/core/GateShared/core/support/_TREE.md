# support/ 目录树

> Gate 的跨模块支撑组件，含与各服务共享 Redis 契约的用户 token 校验器。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | support 层 C++ modules 接口单元。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateRouteModules.h` | 路由模块声明聚合（support 层视图）。 |
| `UserTokenValidator.cpp` | 用户会话 token 校验实现，消费 module 完成请求 guard 与 Redis value 接受判断。 |
| `UserTokenValidator.h` | 用户 token 校验接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
