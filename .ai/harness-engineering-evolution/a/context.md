# Context

Task request:
Absorb good ideas from GitHub/high-star Agent Harness projects, then optimize the base documentation framework and continue evolving MemoChat.

Relevant local files:
- `apps/server/core/AIOrchestrator/docs/harness_engineering_plan.md`
- `apps/server/core/AIOrchestrator/harness/README.md`
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`
- `apps/server/core/AIOrchestrator/harness/llm/service.py`
- `apps/server/core/AIOrchestrator/harness/memory/service.py`
- `apps/server/core/AIOrchestrator/harness/feedback/trace_store.py`

External framework references checked:
- LangGraph: graph-based orchestration, durable execution, human-in-the-loop, memory/persistence.
- OpenAI Agents SDK: agents, tools, handoffs, guardrails, tracing.
- CrewAI: agents, tasks, crews, flows, memory/knowledge concepts.
- Microsoft AutoGen / Agent Framework direction: multi-agent runtime, AgentChat/Core style separation.
- MCP ecosystem: tool schema, subprocess/client lifecycle, capability registry.

Current MemoChat harness state:
- `AgentHarnessService` has a linear run path: resolve skill -> plan -> load memory -> execute tools -> call LLM -> save memory -> evaluate feedback.
- `TraceEvent` and `AgentTrace` already support layer/name/status/timing metadata.
- `ProviderEndpoint` already separates provider, adapter, deployment, model list, and thinking parameter.
- The repo has Qdrant, Neo4j, Ollama, and AIOrchestrator Docker services.

Open risks:
- Docs must not imply third-party framework dependency that is not present.
- Roadmap should stay compatible with current C++ AIServer and QML client APIs.
- No Docker/runtime config changes are required for this documentation pass.
