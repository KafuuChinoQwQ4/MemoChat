# GateShared 分层契约

本文档记录 GateShared 共享层的分层规则与其强制约束,回应架构评审发现
"GateShared 仍然是高风险共享层":它曾把 runtime、路由、传输、缓存、持久化和
内部 client 融合在一起,任何业务特例不断加入,GateShared 就会演化成新的
GateServer——只不过形式变成共享静态库,造成所有服务一起编译、一起发布并形成
隐式耦合。

## 问题

整改前,`GateShared/core` 只产出**一个**融合静态库 `GateInfraCore`,同时打包
六种互不相关的关注点:

- runtime(`AsioIOServicePool`、`GateGlobals`、`Singleton`)
- config(`ConfigMgr`、`const`)
- cache(`RedisMgr`、`RedisPipelines`)
- clients(Auth/Chat/Verify gRPC 客户端)
- persistence(Postgres/Mongo 的 Dao + Mgr)
- support(`BearerAccessAuth`、`UserTokenValidator`)

这带来两个具体耦合:

1. **编译耦合**:改动任一 `.cpp` 都会重编整个归档并重链每个消费者。
2. **链接耦合**:每个消费者被强制链接 libpq + mongoc + bson,即便它根本不碰
   数据库(AI 代理网关声明 "owns no database",R18 也不直接访问 DB)。

框架层 `GateRuntimeCore` 通过 `memochat_configure_gate_app_lib` **PUBLIC** 链接
了整个 `GateInfraCore`,而其 bootstrap `GateDomainServer.cpp` 直接 `#include`
了 `PostgresMgr/MongoMgr/RedisMgr`——尽管 DB 检查本就由运行期 bool 开关控制。
这是"共享库膨胀→隐式耦合"的根因。

## 整改后的分层

`GateInfraCore` 被拆成六个**叶子切片**,各自是独立的 `STATIC` 库,沿真实依赖
DAG 分层:

```
GateInfraRuntime      (base)          — 无 infra 依赖
GateInfraConfig    -> Runtime
GateInfraCache     -> Config                     [hiredis]
GateInfraClients   -> Config                     [grpc/proto]
GateInfraPersistence -> Config             [libpq / mongoc / bson]
GateInfraSupport   -> Cache, Config
```

- **改一个切片不再重编其它切片的 TU**,解决编译耦合。
- **只有 `GateInfraPersistence` 链接重型 DB 外部依赖**,解决链接耦合:不碰
  数据库的服务不再拖入 libpq/mongoc/bson。

`GateInfraCore` 保留为**每个既有消费者引用的目标名**,但改为 `INTERFACE` 门面,
链接全部六个切片。胖领域核心(`GateR18Core`/`GateMediaCore`/`GateCallCore`/
`GateAccountCore`)仍链接 `GateInfraCore`,传递闭包不变;新的、更精简的消费者可
只依赖它实际使用的切片。

### 框架去耦:就绪探针注入

`GateDomainServer.cpp`(编入共享框架 `GateRuntimeCore`)不再 `#include` 任何具体
管理器。启动就绪检查改为由服务 `main()` 注入的 `GateReadinessProbe` 列表驱动:

- 探针契约 `GateReadinessProbe`(`core/runtime/GateReadinessProbe.hpp`)只依赖
  `<functional>`/`<string>`/`<vector>`,与传输和管理器无关。
- 拥有依赖的切片提供探针工厂:`GateInfraPersistence` 提供 Postgres/Mongo 探针,
  `GateInfraCache` 提供 Redis 探针。
- 服务 `main()` 组装自己实际需要的探针传给 `RunGateDomainServer`。

因此框架不再硬链接 persistence/cache 切片。可验证结果:`AIGatewayServer` 的二进制
中 Postgres/mongoc 符号数为 **0**,而 R18/Moments/Call 因真实使用 DB 仍保留。

## 约束(不可回退)

以下规则由 `tests/python/apps/server/core/test_gate_infra_decomposition_contract.py`
静态强制,回退任一条都会使测试失败:

1. 六个切片各自是独立 `STATIC` 库。
2. 切片按 DAG 分层,无环、无侧向依赖(config 依赖 runtime;cache/clients/
   persistence 依赖 config;support 依赖 cache+config)。
3. `GateInfraCore` 必须是 `INTERFACE` 库——**一旦获得编译源就会重新变成共享代码
   汇集点**,即本整改要防止的"新 GateServer"漂移。
4. 重型 DB 外部依赖(Postgres/mongoc/bson)只允许出现在 `GateInfraPersistence`;
   公共切片配置助手 `memochat_configure_gate_infra_slice` 本身不得链接它们。
5. 共享框架 bootstrap(`GateDomainServer.{hpp,cpp}`)不得出现 `PostgresMgr`/
   `MongoMgr`/`RedisMgr` 任一具体管理器名;新增启动关键依赖时,应在其所属切片
   增加探针工厂,而不是在共享框架里加 `#include` + 特例分支。
6. 框架助手 `memochat_configure_gate_app_lib` 只链接精简基座 `GateInfraConfig`,
   不得链接完整门面 `GateInfraCore`。

## 新增业务需求时怎么做

- 需要一个新的启动关键依赖(如新数据库/消息系统):在其**所属切片**加探针工厂,
  服务 `main()` 按需注入。不要在共享框架里加具体管理器 `#include`。
- 领域专属逻辑(某业务的持久化/缓存用法):放进该业务自己的服务目录
  (`<X>Service/`)与其 `Gate<X>Core`,不要放进 `GateShared/core`。
- 只有真正**跨所有网关共享、且不属于任何单一领域**的基础设施才允许进入切片;
  即便如此也要落在正确的分层里,并保持切片彼此不侧向耦合。

## 治理原则(共享层三原则)

拆分只解决了"物理结构",要防止 GateShared 重新长回 GateServer,还需把治理原则
固化成可执行约束。以下三条由
`tests/python/apps/server/core/test_gate_shared_governance_contract.py` 静态强制:

1. **共享稳定机制,不共享具体业务策略。** 连接池(`PostgresPool`/
   `PooledSqlConnection`)、Redis 客户端、配置加载、就绪探针契约这类**不含任何
   业务词汇**的机制可以共享;某业务"如何拼一条 feed SQL""如何算成人校验"这类
   策略不行。测试断言 `PostgresPool` 类体内不得出现 `moment`/`media_`/
   `call_session`/`friend`/`group_`/`r18` 等业务词汇。
2. **共享接口和基础 DTO,不共享跨域事务。** 切片只能被领域依赖(domain → shared),
   反向不行:任何 `GateInfra*` 切片都不得链接 `Gate<X>Domain`/`Gate<X>Core`。
   这样跨域事务无法下沉进共享层。
3. **服务专属代码必须留在所属服务目录。** `GateShared/modules` 只放中立的横切模块
   (health + 框架 runtime),业务路由模块必须在其 `<X>Service/`。

### 已知例外:融合的 `PostgresDao` / `PostgresMgr`(棘轮约束)

`GateShared/core/persistence` 里的 `PostgresDao`(及其门面 `PostgresMgr`)仍是**一个
类**,把 account、call、media、moments、r18 五个域的业务 SQL 混在一起,并通过
`[AccountPostgres]` 直连账号库。这是原则 1/3 的**已知遗留违背**,记录在案:

- 维护者注记已说明:`PostgresMgr` 持有 `PostgresDao` 成员并转发全部方法,
  `PostgresDao` 又跨 `PostgresDao.cpp` / `PostgresDaoAccount.cpp` 两个 .cpp 定义,
  因此它是**单一单元**,不切分类本身就无法按域拆。全量拆分是一个独立工程。
- 每个仍把 SQL 留在共享 DAO 的域,**必须**在其所属服务目录暴露一个持久化 seam
  (`MomentsPersistence`/`MediaPersistence`/`CallPersistence`/`AccountPersistence`)——
  这是 SQL 未来外迁的落点,也保证 handler 不直接穿透宽 DAO。
- **棘轮**:测试冻结了 `PostgresMgr` 当前的公开方法面(基线 42 个方法)。SQL 外迁
  使方法**减少**→ 通过;新增任何方法(新业务策略挤进共享 DAO)→ **失败**,直到
  它要么获得服务自有 DAO,要么基线被有意识地评审后更新。方向只能是"缩小",
  不能是"增长"。
