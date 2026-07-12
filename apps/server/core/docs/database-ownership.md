# 数据库所有权矩阵

本文档登记各后端微服务的数据库与应用角色归属。微服务拆分只有在**本地 ini 配置、
Helm 配置、本所有权矩阵三者一致**时才算真实落地——`test_database_ownership_contract.py`
据此交叉校验。

## PostgreSQL 所有权

| 服务 | 数据库 | 应用角色 |
|------|--------|----------|
| MediaGatewayServer | `memo_media` | `memo_media_app` |
| MomentsGatewayServer | `memo_moments` | `memo_moments_app` |
| CallGatewayServer | `memo_call` | `memo_call_app` |
| RegisterServer | `memo_account` | `memo_account_app` |
| LoginServer | `memo_account` | `memo_account_app` |
| AccountServer | `memo_account` | `memo_account_app` |
| ChatServer ingress | `memo_pg` chat-owned legacy name | `memochat` |
| R18GatewayServer | `memo_account` policy read/write | `memo_account_app` |
| AIServer | `memo_pg` legacy shared | `memochat` |

说明:
- Media/Moments/Call 已拆出独立数据库 + 专属应用角色(最小权限)。
- Register/Login/Account 共享 `memo_account`(账号域),用 `memo_account_app` 角色。
- ChatServer 主库仍是历史名 `memo_pg`(chat 域自有),用 `memochat` 角色;ChatServer ingress
  和独立 chat worker 配置另有 `[AccountPostgres]` 桥接 `memo_account` 以读账号资料。
- R18Gateway 的源内容仍由自身文件/Redis 设施管理；账号域只拥有成人确认与 R18 访问策略，
  因此 R18Gateway 以 `memo_account_app` 访问 `memo_account` 的策略字段。
- AIServer 仍使用 legacy `memo_pg` / `memochat`,属历史遗留共享,未拆分。

## 其他数据存储

- AI 语义缓存(Redis):key 前缀 `memochat:ai:semantic_cache:`。
- 向量库(Qdrant):每用户隔离,使用 Qdrant collection prefix `user_`。
- 图谱(Neo4j):database `neo4j`。

## 一致性校验

本矩阵 token 被 `test_documented_matrix_lists_current_database_owners` 断言;所有权与实际配置
的一致性由同文件其余用例校验(本地 ini `[Postgres]`、ChatServer 的 `[AccountPostgres]` 桥接、
Helm values/ConfigMap 角色、AIOrchestrator 外部存储前缀)。改动任一服务的库/角色归属时,
必须同步更新本矩阵、对应 ini 与 Helm 配置,三者保持一致。
