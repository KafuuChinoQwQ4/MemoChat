# Context

Task: assess AI Agent completion and continue the next useful implementation task.

Relevant files:
- `apps/server/core/AIOrchestrator`: FastAPI harness, LLM registry, tool executor, KB/RAG, graph memory, trace store.
- `apps/server/core/AIServer`: C++ AI service gateway layer.
- `apps/client/desktop/MemoChat-qml/AgentController.h`
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/KnowledgeBasePane.qml`
- `apps/client/desktop/MemoChat-qml/LocalFilePickerService.*`
- `apps/server/migrations/postgresql/business/004_ai_agent.sql`
- `apps/server/migrations/postgresql/business/005_ai_agent_harness.sql`

Docker/MCP checks:
- `docker ps` shows `memochat-ai-orchestrator`, `memochat-ollama`, `memochat-qdrant`, `memochat-neo4j`, `memochat-postgres`, and other infrastructure containers running with stable published ports.
- `GET http://127.0.0.1:8096/health`: ok.
- `GET http://127.0.0.1:8096/models`: returns Ollama provider with `qwen3:4b`, `qwen2.5:3b`, `qwen2.5-coder:3b`; default `qwen3:4b`.
- `GET http://127.0.0.1:8096/agent/skills`: returns harness skills.
- `GET http://127.0.0.1:8096/agent/tools`: returns builtin tools.
- `GET http://127.0.0.1:11434/api/tags`: local Ollama has `qwen3:4b`.
- Qdrant MCP: no collections yet.
- Neo4j MCP: schema has `User`, `AgentSession`, `AgentTurn`, `Topic` labels and agent relationships.
- Postgres MCP: AI tables exist, including session/message/memory/knowledge-base/trace/feedback tables.

Completion assessment:
- Backend harness/API/config/migrations are substantially present.
- Runtime services are up and discoverable.
- Client Agent chat, session, model, and KB controller methods exist.
- Gaps found:
  - `AgentController::onHttpFinish` erases pending request context before handlers read it, breaking response routing state.
  - Controller starts with model `qwen2.5:7b`, but runtime model list does not contain it.
  - `KnowledgeBasePane.qml` exists but is not visible from `AgentPane.qml`.
  - Knowledge-base upload button is a TODO and does not call the existing `uploadDocument`.
  - Controller does not expose a KB list/search-result property to QML.

Open risks:
- Full client runtime UI smoke requires launching the desktop app, not just compilation.
- Backend `/ai/...` gateway compatibility is assumed from existing routes; this task focuses on client wiring.
