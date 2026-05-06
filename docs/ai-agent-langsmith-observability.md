# AI Agent LangSmith 观测系统设计

日期：2026-05-05

MemoChat AIOrchestrator 使用 LangSmith 作为 AI Agent 原生观测主系统。Prometheus/Grafana 保留用于服务和基础设施指标，本地 TraceStore 保留用于前端运行图和历史回放。

## 目标

- 用 LangSmith 记录 Agent、RAG、Tool、LLM、Fallback、Guardrails 的结构化运行轨迹。
- 保留 `/metrics` 输出，继续服务 Prometheus/Grafana。
- 不依赖 LangSmith 可用性来保证业务可用：缺少 SDK 或 API Key 时自动 no-op。
- 默认保护隐私：`uid` 哈希化，`content/query/token/api_key/password` 等敏感字段脱敏。

## 环境变量

```powershell
$env:MEMOCHAT_AI_OBSERVABILITY__ENABLED="true"
$env:LANGSMITH_TRACING="true"
$env:LANGSMITH_API_KEY="<your-langsmith-api-key>"
$env:LANGSMITH_PROJECT="memochat-ai-local"
$env:LANGSMITH_ENDPOINT="https://api.smith.langchain.com"
```

配置文件位置：`apps/server/core/AIOrchestrator/config.yaml`。

官方参考：LangSmith 文档说明 tracing 可通过 `LANGSMITH_TRACING`、`LANGSMITH_API_KEY`、
`LANGSMITH_ENDPOINT`、`LANGSMITH_PROJECT` 配置；`trace` context manager 在 Python SDK 0.1.95
之后会遵守 `LANGSMITH_TRACING`。见 https://docs.langchain.com/langsmith/trace-without-env-vars 。

## Trace 分层

一次 `/agent/run` 在 LangSmith 中按下面的语义拆分：

```text
agent_run
├── agent_input_guardrails
├── agent_memory_load
├── agent_tool_plan_guardrails
├── agent_tool_execution
│   ├── tool_knowledge_search
│   └── tool_<name>
├── agent_model_completion
│   ├── llm_completion_attempt
│   └── ollama_chat
├── agent_output_guardrails
└── agent_memory_writeback
```

RAG 知识库链路：

```text
kb_upload
└── rag_add_documents

kb_search
└── rag_retrieve
```

## Metadata 约定

所有 LangSmith run 默认带：

- `service=AIOrchestrator`
- `env=local|...`
- `route`
- `session_id`
- `local_trace_id`
- `skill`
- `model_name`
- `deployment_preference`
- `tool`
- `collection`
- `score_threshold`

隐私字段默认处理：

- `uid` -> `uid_hash`
- `content` -> `[redacted]`
- `query` -> `[redacted]`
- `token/api_key/password/secret/cookie/authorization` -> `[redacted]`

如果需要把用户输入上传到 LangSmith 便于调试，可以在本地临时设置：

```yaml
observability:
  langsmith:
    upload_user_content: true
```

## 与现有观测的关系

- LangSmith：AI 语义观测、Prompt/Tool/RAG/LLM trace、评测数据集、实验对比。
- Prometheus/Grafana：HTTP 请求数、Ollama retry、健康状态、容器 CPU/内存。
- Loki/Tempo/OpenTelemetry：系统级日志和分布式链路辅助。
- 本地 TraceStore：MemoChat UI 的 `/agent/runs/{trace_id}` 和运行图。

## Eval 规划

建议建立两个 LangSmith dataset：

- `memochat-rag-regression`：验证知识库召回、top-k、答案包含预期词。
- `memochat-agent-regression`：验证技能选择、工具选择、guardrail 拦截、模型 fallback。

核心指标：

- `rag_recall_at_k`
- `answer_contains_expected_terms`
- `tool_selection_accuracy`
- `guardrail_block_accuracy`
- `latency_ms`
- `tokens_total`
- `fallback_count`

## 关键文件

- `apps/server/core/AIOrchestrator/observability/langsmith_instrument.py`
- `apps/server/core/AIOrchestrator/config.py`
- `apps/server/core/AIOrchestrator/config.yaml`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/api/kb_router.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`
- `apps/server/core/AIOrchestrator/rag/chain.py`
- `apps/server/core/AIOrchestrator/llm/manager.py`
- `apps/server/core/AIOrchestrator/llm/ollama_llm.py`
