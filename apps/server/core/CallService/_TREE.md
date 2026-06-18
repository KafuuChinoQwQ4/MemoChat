# CallService/ 目录树

> 通话信令域服务，从 GateServer 剥离（Phase 4）；持有通话数据（Postgres）、Redis call:* 键与 LiveKit token 签发，对外提供 /api/call/*。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | CallGatewayServer 进程入口 |
| [`core/`](core/_TREE.md) | 通话服务核心支撑逻辑 |
| [`domain/`](domain/_TREE.md) | 通话路由领域模块与路由 Profile |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | CallService 构建脚本 |
| `callgateway.ini` | CallGatewayServer 运行配置 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
