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
  **隔离缝(finding #3 option C)**:业务代码不得再直接调用 `PostgresMgr::GetUser` /
  `GetUidByUserId` 等账号读;全部经 `IAccountDirectory`(`ChatAccountDirectory`
  适配器,今日仍走 `[AccountPostgres]` 桥接,行为不变)。后续换 gRPC 快照 API 或
  事件投影时只改适配器。契约见
  `tests/python/apps/server/core/ChatServer/test_account_directory_seam_contract.py`。
- R18Gateway 的源内容仍由自身文件/Redis 设施管理；账号域只拥有成人确认与 R18 访问策略，
  因此 R18Gateway 以 `memo_account_app` 访问 `memo_account` 的策略字段。
- AIServer 仍使用 legacy `memo_pg` / `memochat`,属历史遗留共享,未拆分。

## 账号域:一个 bounded context,多个部署角色

Register / Login / Account **不是三个自治的数据服务,而是同一个账号 bounded context
的三个部署角色**。共享 `memo_account` 库在此前提下是可接受的,但必须由契约锁死,防止
三者演化成各自维护不兼容 SQL 的独立数据服务(会形成数据库级耦合)。现状(由
`test_account_bounded_context_contract.py` 静态强制)如下:

- **单一 DAL,零分叉 SQL**:三个服务目录(`RegisterService/`、`LoginService/`、
  `AccountService/`)内**没有任何 raw SQL / pqxx 调用**。账号表 SQL 只存在于共享
  `GateShared/core/persistence/PostgresDaoAccount.cpp`(+ `PostgresDao.cpp` 的
  `UpdateUserProfile`),域门面是 `AccountShared/.../account/AccountPersistence`。
- **单一库/角色/迁移集**:三份 ini 都锁 `[Postgres] Database=memo_account`、
  `User=memo_account_app`,且**均不声明 `[AccountPostgres]` 桥接**(它们*拥有*该表,
  经自身连接池直读,与 ChatServer 的跨库桥接相反)。schema 由单一迁移集
  `migrations/postgresql/business/009_memo_account_schema.sql`(+ `010/011/013`)拥有,
  无 per-service 迁移目录。
- **跨服务共写表 `auth_refresh_token` 的串行化契约**:这是唯一被两个角色写入的表——
  Login 签发/轮换、Account 吊销。写入全部经 `AccountPersistence` 的 refresh-token 方法,
  落到 `PostgresDaoAccount.cpp` 单一文件,并由 `SELECT … FOR UPDATE` 行锁串行化
  (rotate / revoke 路径)。新增对该表的写入**必须**走同一 DAL 方法,不得旁路,否则
  破坏行锁纪律。

已知后续项(不阻塞上述契约):`AccountShared` 内有 5 处
(`Http2ProfileSupport.cpp`、`ProfileRouteModule.cpp`、`AuthLoginSupport.cpp`)直接调用
`PostgresMgr` 账号方法而非经 `AccountPersistence` 门面。它们仍走同一实现、同一 SQL(非
分叉),属分层一致性瑕疵,宜后续统一收口到 `AccountPersistence`。

## 其他数据存储

- AI 语义缓存(Redis):key 前缀 `memochat:ai:semantic_cache:`。
- 向量库(Qdrant):每用户隔离,使用 Qdrant collection prefix `user_`。
- 图谱(Neo4j):database `neo4j`。

## 一致性校验

本矩阵 token 被 `test_documented_matrix_lists_current_database_owners` 断言;所有权与实际配置
的一致性由同文件其余用例校验(本地 ini `[Postgres]`、ChatServer 的 `[AccountPostgres]` 桥接、
Helm values/ConfigMap 角色、AIOrchestrator 外部存储前缀)。改动任一服务的库/角色归属时,
必须同步更新本矩阵、对应 ini 与 Helm 配置,三者保持一致。
