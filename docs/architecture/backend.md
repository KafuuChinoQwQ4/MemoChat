# 服务端模块

服务端聚合根位于 `apps/server/`，核心源码位于 `apps/server/core/`。顶层 CMake 在 `BUILD_SERVER` 或 `BUILD_TESTS` 开启时加入服务端核心。服务端默认使用 C++26；实验性 reflection 和 GNU modules 由 preset/目标开关控制。

## 聚合目录

| 路径 | 说明 |
| --- | --- |
| `core/` | 各后端微服务核心源码 |
| `config/` | 服务端运行期 ini 配置 |
| `migrations/` | 数据库结构迁移脚本，当前主要是 PostgreSQL |
| `Memo_ops/` | 服务端运维侧运行期产物和 artifacts 空间 |
| `ai-workspace/` | AI 实验性模块占位工作区 |

## 主要服务目标

| 目标 | 路径 | 说明 |
| --- | --- | --- |
| `ChatServer` | `ChatServer/` | 聊天入口、会话、消息收发、历史、心跳和多传输接入 |
| `ChatDeliveryWorker` | `ChatServer/` | 聊天投递相关 worker |
| `ChatRelationQueryService` | `ChatServer/` | 聊天层关系查询服务 |
| `ChatRelationServiceWorker` | `ChatServer/` | 聊天关系服务 worker |
| `ChatMessageService` | `ChatServer/` | 消息服务拆分目标 |
| `RegisterServer` | `RegisterService/` | 注册域服务 |
| `LoginServer` | `LoginService/` | 登录域服务 |
| `AccountServer` | `AccountService/` | 账号资料域服务 |
| `MediaGatewayServer` | `MediaService/` | 媒体上传、下载、对象存储访问 |
| `MomentsGatewayServer` | `MomentsService/` | 朋友圈域网关 |
| `CallGatewayServer` | `CallService/` | 音视频通话域网关 |
| `AIGatewayServer` | `AIGatewayService/` | AI HTTP 网关，转接 AIOrchestrator |
| `R18GatewayServer` | `R18Service/` | R18 分区网关 |
| `VarifyServer` | `VarifyServer/` | 邮箱验证码服务 |
| `AIServer` | `AIServer/` | C++ AI 侧服务目标 |

`GateShared/` 提供拆分后网关共享基础设施、传输、插件和路由组件。数据库所有权以 `apps/server/core/docs/database-ownership.md` 为准。

`common/` 放服务端共享组件，例如认证、集群、数据库、JSON、日志、Lua、proto、reflection 和 runtime 辅助。跨服务共享逻辑优先进入明确的 common 子域或 GateShared/AccountShared 这类有边界的共享库，不要随意让单个服务目录承担全局职责。

## 开发边界

- HTTP 入口、认证、媒体、朋友圈、通话、AI 网关按服务拆分，避免回到单个 GateServer 大中枢。
- ChatServer 是聊天长连接和消息域，不应承接账号、媒体、AI 的业务规则。
- 配置、迁移和数据库所有权必须一起改；不要只改源码而遗漏 `apps/server/config/`、`apps/server/migrations/` 或模块内 docs。
- 密钥和生产注入规则见 `apps/server/core/docs/secret-management.md`。
- 修改服务库/角色归属时，同步 C++/ini/Helm 配置和 `database-ownership.md`。
