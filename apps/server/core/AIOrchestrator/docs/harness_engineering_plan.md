# AI Harness Engineering Plan

## 目标

将 `server/AIOrchestrator` 升级为 MemoChat 唯一的 AI Agent Harness 根目录，并以 2026 工业级目标为约束构建后端框架：

- 支持 `external_api` 与 `local_api` 两类 LLM 接口
- 在一个根目录下按层次拆分：`orchestration`、`execution`、`feedback`、`memory`
- 集成 MCP、Skill Registry、Knowledge Base、Neo4j Graph Memory、Qdrant RAG
- 提供可追踪、可反馈、可扩展、可 Docker 化的运行基线

## 当前落地结构

```text
server/AIOrchestrator/
  harness/
    orchestration/
    execution/
    feedback/
    memory/
    knowledge/
    llm/
    mcp/
    skills/
    runtime/
```

## 分层说明

### 编排层

- `harness/orchestration/planner.py`
- `harness/orchestration/agent_service.py`

职责：

- 解析 skill
- 生成执行计划
- 汇总记忆、工具观察、模型执行
- 产出统一 trace

### 执行层

- `harness/execution/tool_executor.py`
- `harness/llm/service.py`

职责：

- 调 LLM
- 调内置工具 / MCP 工具
- 组织知识库、图谱、搜索、计算等执行动作

### 反馈层

- `harness/feedback/trace_store.py`
- `harness/feedback/evaluator.py`

职责：

- 记录 run trace / step trace
- 记录用户反馈
- 生成自动反馈摘要

### 记忆层

- `harness/memory/service.py`
- `harness/memory/graph_memory.py`

职责：

- 短期会话记忆
- 情景记忆
- 语义偏好记忆
- 图谱记忆装载与交互写回

## API 设计

兼容旧接口：

- `POST /chat`
- `POST /chat/stream`
- `POST /smart`
- `POST /kb/upload`
- `POST /kb/search`
- `GET /kb/list`
- `DELETE /kb/{kb_id}`
- `GET /models`

新增 Harness 管理接口：

- `POST /agent/run`
- `GET /agent/runs/{trace_id}`
- `POST /agent/feedback`
- `GET /agent/skills`
- `GET /agent/tools`

## Docker 策略

`server/AIOrchestrator/docker-compose.yml` 现在包含：

- `memochat-ai-orchestrator`: harness 服务本体
- `memochat-ollama`: 本地 `local_api`
- `memochat-qdrant`: 向量库
- `memochat-neo4j`: 图谱库

AI 原生观测使用官方 LangSmith 云端服务，通过 `LANGSMITH_API_KEY`、`LANGSMITH_PROJECT` 和
`LANGSMITH_ENDPOINT` 环境变量接入；Prometheus/Grafana 与 OpenTelemetry 继续承担服务级指标和链路辅助。

远端 `external_api` 通过 `config.yaml` 中的 OpenAI/Kimi/Claude 或自定义 `harness.providers.endpoints` 接入。

配置支持通过环境变量覆盖，格式为：

- `MEMOCHAT_AI_LLM__OLLAMA__BASE_URL`
- `MEMOCHAT_AI_RAG__QDRANT__HOST`
- `MEMOCHAT_AI_NEO4J__HOST`
- `MEMOCHAT_AI_POSTGRES__HOST`

## 下一阶段

1. 将图谱写入、Graph recall 和推荐能力继续收敛到新 harness
2. 为 `/agent/run` 增加更细粒度的计划节点和 LangGraph 编排
3. 引入 run replay / benchmark / regression tests
4. 为 MCP 显式工具调用补齐参数模板和前端入口
