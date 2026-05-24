---
name: memochat-db-migration
description: Use when changing MemoChat Postgres, Redis, MongoDB, MinIO metadata, Neo4j, Qdrant schema, migrations, indexes, seed data, cache keys, persistence, or storage metadata.
---

# MemoChat 数据库迁移

当修改 schema、迁移、索引、种子数据、持久化逻辑、缓存 key、图记忆、向量集合或存储元数据时使用。

## 范围

相关位置：

- `apps/server/migrations/postgresql`
- 旧版 Windows 初始化 helper：`tools/scripts/init_postgresql_schema.ps1`
- `tools/scripts/fix_postgresql_sequences.sql`
- `tools/scripts/pg_*.sql`
- `apps/server/core/*`
- `apps/server/config`
- `infra/deploy/local/init`
- `apps/server/core/AIOrchestrator/graph`
- `apps/server/core/AIOrchestrator/rag`

## 规则

- 通过 Docker 或 MCP 查询数据库。
- 不要使用本地主机安装的数据库。
- Postgres 主机访问保持在 `127.0.0.1:15432`；容器内 Postgres 使用 `5432`。
- 保持变更具备向后意识：配置、初始化脚本、迁移、运行时代码和测试必须一致。
- 除非用户明确批准具体 volume 或主机数据路径，否则绝不重置 Docker volumes。当前 `archlinux` 绑定数据位于 `/data/docker-data/memochat`；旧 Docker Desktop 数据位于 `D:\docker-data\memochat`，仅作为旧版备份/源数据。

## 发现

检查当前状态：

```bash
docker exec memochat-postgres psql -U memochat -d memo_pg -c "\dn"
docker exec memochat-postgres psql -U memochat -d memo_pg -c "\dt memo.*"
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select version();"
docker exec memochat-redis redis-cli -a 123456 info keyspace
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.getSiblingDB('memochat').getCollectionNames()"
```

可用时使用 MCP 检查 Neo4j/Qdrant：

- `get_neo4j_schema`
- `read_neo4j_cypher`
- `qdrant_list_collections`
- `qdrant_scroll`

## 迁移计划

对每个数据变更记录：

- 所属服务/模块
- 受影响的 table/collection/key/queue/object bucket
- 要更新的迁移或初始化文件
- 读取/写入它的运行时代码路径
- 回滚、兼容性和幂等性顾虑
- 验证查询

兼容性检查必须回答：

- 旧服务读取新 schema 是否会失败。
- 新服务读取旧数据是否有默认值或迁移路径。
- 迁移重复执行是否安全。
- 初始化脚本和已有持久化数据是否都会得到同一最终结构。
- 回滚时是可逆 SQL、前向修复，还是需要明确用户批准的数据操作。

## 实现

- 对持久化 Postgres 变更添加 SQL 迁移。
- 如果新鲜 Docker 数据必须包含该变更，更新初始化脚本。
- 同步更新 C++/Python 数据访问代码和 config key。
- 对 Redis key，定义 TTL 和 namespace。
- 对 Mongo collection，记录索引和所有权。
- 对 Qdrant collection，记录向量维度、距离度量、payload schema 和删除策略。
- 对 Neo4j，记录 labels、relationships、constraints 和写路径。

## 验证

在任何 `archlinux` WSL 运行时或部署验证前，先使用 Linux server 构建：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

然后通过 Docker/MCP 运行定向数据库查询。对于运行时路径，使用：

```bash
tools/scripts/status/deploy_services.sh
tools/scripts/status/start-all-services.sh
```

验证至少覆盖：

- 新鲜初始化路径。
- 已有数据迁移路径。
- 关键读写路径。
- 重复执行迁移或初始化片段的幂等性；不能安全重复时，记录保护条件。

有用时仍可从 Windows 运行旧版探针：

```powershell
tools/scripts/test_register_login.ps1
tools/scripts/test_login.ps1
```

## 报告

包含：

- 修改的文件
- schema/key/collection 变更
- 迁移/初始化覆盖情况
- 验证查询和结果
- 是否需要数据重置
