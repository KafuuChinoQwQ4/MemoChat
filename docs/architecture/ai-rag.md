# AI / RAG 模块

AIOrchestrator 位于 `apps/server/core/AIOrchestrator/`，是 Python FastAPI 服务，负责普通对话、流式输出、知识库、模型管理、Agent harness、桌宠 AI 能力和观测接入。C++ 侧的 `AIGatewayServer` 负责网关转接，`AIServer` 是 C++ AI 侧服务目标。

## FastAPI 路由

`AIOrchestrator/main.py` 注册的主要路由前缀：

| 前缀 | 职责 |
| --- | --- |
| `/chat` | 普通对话和 SSE 流式对话 |
| `/smart` | 摘要、建议回复、翻译等智能处理 |
| `/kb` | 知识库上传、搜索、列表、删除 |
| `/models` | 模型和 provider 管理 |
| `/recommend` | 图谱/推荐相关能力 |
| `/agent` | Agent harness、A2A、任务运行和流式运行 |
| `/pet` | 桌宠视觉、语音、训练和运行时能力 |
| `/health`、`/ready`、`/metrics` | 健康检查和指标 |

## 目录职责

| 目录 | 职责 |
| --- | --- |
| `api/` | FastAPI router |
| `llm/` | OpenAI/Kimi/Ollama/Claude 等模型适配 |
| `rag/` | 文档处理、embedding、检索和 RAG chain |
| `tools/` | Agent 工具注册、MCP bridge、知识库工具等 |
| `harness/` | Agent orchestration、interop、pet runtime 等工程化能力 |
| `db/`、`graph/` | Postgres 和 Neo4j 访问 |
| `observability/` | metrics、tracing、LangSmith 接入 |

## 数据依赖

AI 相关依赖包括 Postgres、Redis、RabbitMQ、Neo4j、Qdrant、模型 provider 和对象/本地文件缓存。生产凭据必须通过 `MEMOCHAT_AI_*` 环境变量或 Secret 注入，具体见 `apps/server/core/docs/secret-management.md`。
