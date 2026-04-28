# MemoChat AI Agent 端到端测试指南

> 当前版本：2026-04-26  
> 相关目录：`apps/server/core/AIOrchestrator`、`apps/server/core/AIServer`

AI 能力由 `AIServer` 和 Docker 中的 `AIOrchestrator` 协作完成。`AIOrchestrator` 再连接 Docker 内的 Ollama、Qdrant、Neo4j，以及宿主机映射出的 Docker Postgres。

## 1. 架构边界

| 组件 | 运行形态 | 端口 | 职责 |
|------|----------|------|------|
| `AIServer` | Windows C++ 进程 | 由配置决定 | 和主服务集成的 AI 桥接层 |
| `AIOrchestrator` | Docker 容器 `memochat-ai-orchestrator` | `8096` | LLM/RAG/工具/图谱编排 |
| `Ollama` | Docker 容器 `memochat-ollama` | `11434` | 本地模型推理 |
| `Qdrant` | Docker 容器 `memochat-qdrant` | `6333`, `6334` | 向量库 |
| `Neo4j` | Docker 容器 `memochat-neo4j` | `7474`, `7687` | 图谱记忆 |
| `PostgreSQL` | Docker 容器 `memochat-postgres` | `15432` | 用户、会话、AI 元数据 |

AIOrchestrator 在容器内访问 Postgres 时使用：

```text
MEMOCHAT_AI_POSTGRES__HOST=host.docker.internal
MEMOCHAT_AI_POSTGRES__PORT=15432
```

访问 Ollama/Qdrant/Neo4j 时优先使用 Docker 网络内的容器名。

## 2. 测试前检查

### 2.1 容器状态

```powershell
docker ps --format "table {{.Names}}\t{{.Ports}}\t{{.Status}}" | findstr /I "ai-orchestrator ollama qdrant neo4j postgres"
```

期望看到：

- `memochat-ai-orchestrator`
- `memochat-ollama`
- `memochat-qdrant`
- `memochat-neo4j`
- `memochat-postgres`

### 2.2 健康检查

```powershell
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:8096/health
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:11434/api/tags
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:6333/collections
Invoke-WebRequest -UseBasicParsing http://127.0.0.1:7474
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
```

### 2.3 模型检查

```powershell
docker exec memochat-ollama ollama list
```

如果缺少模型，在 Docker 容器内拉取：

```powershell
docker exec memochat-ollama ollama pull qwen2.5:7b
docker exec memochat-ollama ollama pull nomic-embed-text
```

如果当前配置使用的是其他模型名，以 `apps/server/core/AIOrchestrator/config.yaml` 为准。

## 3. 配置检查

查看容器环境变量：

```powershell
docker inspect memochat-ai-orchestrator --format "{{range .Config.Env}}{{println .}}{{end}}" | findstr /I "POSTGRES OLLAMA QDRANT NEO4J"
```

关键项：

```text
MEMOCHAT_AI_POSTGRES__HOST=host.docker.internal
MEMOCHAT_AI_POSTGRES__PORT=15432
MEMOCHAT_AI_LLM__OLLAMA__BASE_URL=http://memochat-ollama:11434
MEMOCHAT_AI_RAG__QDRANT__HOST=memochat-qdrant
MEMOCHAT_AI_NEO4J__HOST=memochat-neo4j
```

本地配置文件：

```text
apps/server/core/AIOrchestrator/config.yaml
apps/server/core/AIOrchestrator/config.py
apps/server/core/AIServer/config.ini
```

## 4. 端到端测试用例

### TC-AI-001 普通对话

步骤：

1. 启动 Docker AI 依赖和 Windows 业务服务。
2. 登录 MemoChat 客户端。
3. 进入 AI 助手页面。
4. 输入“你好，请介绍一下自己”。
5. 等待回复。

预期：

- UI 不崩溃。
- AIOrchestrator 返回 2xx。
- 回复内容是中文或符合当前模型配置。
- 日志中能看到同一个 `trace_id` 或请求上下文。

### TC-AI-002 RAG 检索

步骤：

1. 准备一个小型 `.md` 或 `.txt` 文件。
2. 上传到知识库。
3. 等待切片和向量写入 Qdrant。
4. 输入与文档内容相关的问题。

预期：

- Qdrant collection 中有向量。
- AI 回复引用或利用文档内容。
- 上传失败时返回可读错误，不影响普通聊天。

### TC-AI-003 图谱记忆

步骤：

1. 发起一段包含人物、项目、关系的对话。
2. 触发图谱写入或记忆抽取。
3. 通过 Neo4j Browser 或 Cypher 查询节点/关系。

检查：

```powershell
docker exec memochat-neo4j cypher-shell -u neo4j -p 123456 "MATCH (n) RETURN count(n) LIMIT 1"
```

预期：

- Neo4j 可写入和查询。
- 图谱失败不应中断普通 AI 回复。

### TC-AI-004 聊天摘要

步骤：

1. 在普通聊天中产生多条消息。
2. 点击摘要能力。
3. 等待 AI 生成摘要。

预期：

- 摘要与当前对话相关。
- 不泄露无关用户数据。
- 摘要失败有明确错误提示。

### TC-AI-005 智能回复建议

步骤：

1. 打开私聊或群聊。
2. 请求建议回复。
3. 观察返回数量和文本长度。

预期：

- 返回多条候选。
- 语气和上下文匹配。
- 不阻塞聊天主线程。

### TC-AI-006 翻译

步骤：

1. 选择一条英文或日文消息。
2. 调用翻译能力。

预期：

- 返回目标语言翻译。
- 失败不影响原消息展示。

### TC-AI-007 模型不可用

步骤：

1. 暂停 Ollama 容器：

```powershell
docker stop memochat-ollama
```

2. 发送 AI 请求。
3. 恢复 Ollama：

```powershell
docker start memochat-ollama
```

预期：

- AI 请求失败时提示明确。
- GateServer/客户端不崩溃。
- 恢复后可继续使用。

## 5. 性能目标

| 指标 | 本地目标 |
|------|----------|
| AIOrchestrator 健康检查 | < 500ms |
| 非流式普通问答 | < 10s，取决于本地模型 |
| 首 token 时间 | < 3s，取决于模型和硬件 |
| Qdrant 检索 | < 500ms |
| 图谱查询 | < 1s |

## 6. 日志与排障

AIOrchestrator 容器日志：

```powershell
docker logs --tail=200 memochat-ai-orchestrator
```

Ollama 日志：

```powershell
docker logs --tail=200 memochat-ollama
```

业务服务日志：

```text
infra/Memo_ops/artifacts/logs/services
```

常见问题：

| 问题 | 优先检查 |
|------|----------|
| AI 回复为空 | Ollama 模型是否存在、AIOrchestrator 日志 |
| 知识库上传失败 | Qdrant 是否健康、文件大小和格式、Postgres 元数据连接 |
| 图谱写入失败 | Neo4j 用户密码、Bolt 端口 `7687`、容器健康 |
| 容器访问 Postgres 失败 | `host.docker.internal:15432` 是否可用 |
| 客户端网络错误 | GateServer `/healthz`、AIServer/AIOrchestrator 路由 |

## 7. 测试报告模板

```text
测试日期：
代码版本：
Docker 容器版本：
模型：

| 用例 | 结果 | 备注 |
|------|------|------|
| TC-AI-001 普通对话 | PASS/FAIL | |
| TC-AI-002 RAG 检索 | PASS/FAIL | |
| TC-AI-003 图谱记忆 | PASS/FAIL | |
| TC-AI-004 聊天摘要 | PASS/FAIL | |
| TC-AI-005 智能回复建议 | PASS/FAIL | |
| TC-AI-006 翻译 | PASS/FAIL | |
| TC-AI-007 模型不可用 | PASS/FAIL | |
```
