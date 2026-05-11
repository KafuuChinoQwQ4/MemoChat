---
description: Work on MemoChat AIOrchestrator, Ollama, Qdrant RAG, Neo4j graph memory, MCP bridge, and knowledge-base flows.
---

# MemoChat AI RAG

Use for AI Orchestrator, LLM routing, tools, RAG, knowledge base, graph memory, Qdrant, Neo4j, Ollama, and MCP bridge work.

## Relevant Areas

- `apps/server/core/AIOrchestrator`
- `apps/server/core/AIServer`
- `tools/mcps/user-qdrant`
- `tools/mcps/user-neo4j`
- `tools/mcps/user-docker-services`
- `apps/server/core/AIOrchestrator/docker-compose.yml`
- `apps/server/core/AIOrchestrator/config.yaml`

AI bind data defaults to `/data/docker-data/memochat/ai-orchestrator` under Arch native Docker. Use Docker Desktop paths only for legacy migration/backup checks.

## Docker Services

Expected services and ports:

- AI Orchestrator `8096`
- Ollama `11434`
- Qdrant `6333/6334`
- Neo4j `7474/7687`

Check:

```bash
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
curl -fsS http://127.0.0.1:8096/health
curl -fsS http://127.0.0.1:11434/api/tags
curl -fsS http://127.0.0.1:6333/
curl -fsS http://127.0.0.1:7474/
```

Use MCP for Qdrant/Neo4j when possible:

- `qdrant_list_collections`
- `qdrant_scroll`
- `qdrant_search`
- `get_neo4j_schema`
- `read_neo4j_cypher`

## Change Checklist

For LLM routing:

- config defaults
- provider enable flags
- timeout/retry behavior
- error logging without leaking secrets

For RAG:

- document chunking
- embedding backend and vector size
- Qdrant collection naming
- payload schema
- deletion/cleanup path

For Neo4j memory:

- labels and relationships
- constraints/indexes
- write path and read path
- idempotency

For MCP bridge:

- subprocess lifecycle
- timeout handling
- tool schema validation
- stderr/log capture

## Verification

Run Python-level checks where available, then runtime probes:

```bash
python -m compileall apps/server/core/AIOrchestrator
docker logs --tail 100 memochat-ai-orchestrator
```

For C++ AIServer changes:

```bash
source /root/.memochat-linux-env
cmake --preset linux-server-gcc16
cmake --build --preset linux-server-gcc16 --parallel 12
```

## Report

Include:

- AI service/component touched
- config changes
- Qdrant/Neo4j schema or collection impact
- MCP tools used
- runtime probe results
