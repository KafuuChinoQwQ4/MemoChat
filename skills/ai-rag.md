---
name: memochat-ai-rag
description: Use when working on MemoChat AIOrchestrator, Ollama, Qdrant RAG, Neo4j graph memory, MCP bridge, knowledge bases, LLM routing, or AI tool contracts.
---

# MemoChat AI RAG

用于 AI Orchestrator、LLM 路由、工具、RAG、知识库、图记忆、Qdrant、Neo4j、Ollama 和 MCP bridge 工作。

## 相关区域

- `apps/server/core/AIOrchestrator`
- `apps/server/core/AIServer`
- `tools/mcps/user-qdrant`
- `tools/mcps/user-neo4j`
- `tools/mcps/user-docker-services`
- `apps/server/core/AIOrchestrator/docker-compose.yml`
- `apps/server/core/AIOrchestrator/config.yaml`

AI 绑定数据在 `archlinux` WSL 的 Arch 原生 Docker 下默认位于 `/data/docker-data/memochat/ai-orchestrator`。Docker Desktop 路径仅用于旧版迁移/备份检查。

## Docker 服务

预期服务和端口：

- AI Orchestrator `8096`
- Ollama `11434`，仅当本地启用 Ollama 时要求存在
- Qdrant `6333/6334`
- Neo4j `7474/7687`

检查：

```bash
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
curl -fsS http://127.0.0.1:8096/health
curl -fsS http://127.0.0.1:11434/api/tags  # only when Ollama is enabled
curl -fsS http://127.0.0.1:6333/
curl -fsS http://127.0.0.1:7474/
```

可用时使用 MCP 检查 Qdrant/Neo4j：

- `qdrant_list_collections`
- `qdrant_scroll`
- `qdrant_search`
- `get_neo4j_schema`
- `read_neo4j_cypher`

## 变更清单

LLM 路由：

- config 默认值
- provider enable flags
- timeout/retry 行为
- 错误日志不泄露 secrets

RAG：

- 文档切分
- embedding backend 和向量维度
- Qdrant collection 命名
- payload schema
- 删除/清理路径

Neo4j memory：

- labels 和 relationships
- constraints/indexes
- 写路径和读路径
- 幂等性

MCP bridge：

- subprocess 生命周期
- timeout 处理
- tool schema 校验
- stderr/log 捕获

## 验证

先运行可用的 Python 层检查，再做运行时探针：

```bash
python -m compileall apps/server/core/AIOrchestrator
docker logs --tail 100 memochat-ai-orchestrator
```

对于 C++ AIServer 变更：

```bash
source /root/.memochat-linux-env
cmake --preset linux-full-gcc16
cmake --build --preset linux-full-gcc16 --parallel 12
```

## 报告

包含：

- 触及的 AI 服务/组件
- 配置变更
- Qdrant/Neo4j schema 或 collection 影响
- 使用过的 MCP 工具
- 运行时探针结果
