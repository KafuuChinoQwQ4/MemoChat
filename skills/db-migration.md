---
description: Plan and verify MemoChat database changes across Postgres, Redis, MongoDB, MinIO metadata, Neo4j, and Qdrant using Docker/MCP-backed checks.
---

# MemoChat Database Migration

Use when changing schemas, migrations, indexes, seed data, persistence logic, cache keys, graph memory, vector collections, or storage metadata.

## Scope

Relevant locations:

- `apps/server/migrations/postgresql`
- `tools/scripts/init_postgresql_schema.ps1`
- `tools/scripts/fix_postgresql_sequences.sql`
- `tools/scripts/pg_*.sql`
- `apps/server/core/*`
- `apps/server/config`
- `infra/deploy/local/init`
- `apps/server/core/AIOrchestrator/graph`
- `apps/server/core/AIOrchestrator/rag`

## Rules

- Query databases through Docker or MCP.
- Do not use local host-installed databases.
- Keep Postgres host access on `127.0.0.1:15432`; in-container Postgres uses `5432`.
- Keep changes backward-aware: config, init scripts, migrations, runtime code, and tests must agree.
- Never reset Docker volumes unless the user explicitly approves the exact `D:\docker-data\memochat\...` path.

## Discovery

Check current state:

```powershell
docker exec memochat-postgres psql -U memochat -d memo_pg -c "\dn"
docker exec memochat-postgres psql -U memochat -d memo_pg -c "\dt memo.*"
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select version();"
docker exec memochat-redis redis-cli -a 123456 info keyspace
docker exec memochat-mongo mongosh -u root -p 123456 --authenticationDatabase admin --quiet --eval "db.getSiblingDB('memochat').getCollectionNames()"
```

Use MCP for Neo4j/Qdrant when available:

- `get_neo4j_schema`
- `read_neo4j_cypher`
- `qdrant_list_collections`
- `qdrant_scroll`

## Migration Plan

For each data change, document:

- owning service/module
- table/collection/key/queue/object bucket affected
- migration or init file to update
- runtime code path that reads/writes it
- rollback or compatibility concern
- verification query

## Implementation

- Add SQL migrations for persistent Postgres changes.
- Update init scripts if fresh Docker data must include the change.
- Update C++/Python data access code and config keys together.
- For Redis keys, define TTL and namespace.
- For Mongo collections, document indexes and ownership.
- For Qdrant collections, document vector size, distance metric, payload schema, and deletion policy.
- For Neo4j, document labels, relationships, constraints, and write paths.

## Verification

Use the full local build before any runtime or deploy verification:

```powershell
cmake --preset msvc2022-full
cmake --build --preset msvc2022-full
```

Then run targeted database queries through Docker/MCP. For runtime paths, use:

```powershell
tools\scripts\status\deploy_services.bat
tools\scripts\status\start-all-services.bat
tools\scripts\test_register_login.ps1
tools\scripts\test_login.ps1
```

## Report

Include:

- files changed
- schema/key/collection changes
- migration/init coverage
- verification queries and results
- data reset requirement, if any
