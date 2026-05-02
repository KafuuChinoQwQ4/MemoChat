# MemoChat AI Agent System Roadmap

## Goal

Design and evolve MemoChat's AI Agent as a market-aligned, production-ready system: not just a chat box, but a trustworthy task assistant with durable execution, tools, memory, RAG, traceability, replay, and future agent-to-agent interoperability.

This plan is framework-informed but project-owned. MemoChat should absorb good ideas from LangGraph, OpenAI Agents SDK, CrewAI, MCP, and A2A without binding itself to any one framework prematurely.

## Product North Star

MemoChat AI Agent should become:

- A chat companion for quick answers.
- A task runner for long-running user requests.
- A memory-aware personal assistant.
- A knowledge worker over user-uploaded files and chat history.
- A traceable, debuggable, recoverable agent system.
- A future participant in external tool/agent ecosystems through MCP and, later, A2A-compatible metadata.

The first product milestone is:

> A user can ask MemoChat AI to complete a task, watch every meaningful step, approve sensitive actions, recover after failure, and replay/debug the run later.

## Market References To Absorb

### LangGraph

Absorb:

- Durable execution.
- Explicit graph nodes and edges.
- Checkpointing and resume.
- Human-in-the-loop.
- Long-running task recovery.

MemoChat adaptation:

- Build a lightweight `RunGraph` first.
- Persist node status and safe summaries in `TraceStore`.
- Keep `/chat` fast and simple; use graph runtime for `/agent/run`.

Reference:
- https://docs.langchain.com/oss/python/langgraph/durable-execution
- https://github.com/langchain-ai/langgraph

### OpenAI Agents SDK

Absorb:

- Agents as instructions + tools + model config.
- Handoffs between specialized agents.
- Guardrails with tripwire-style blocking.
- Trace spans for model calls, tools, guardrails, and handoffs.

MemoChat adaptation:

- Add `AgentSpec`, `ToolSpec`, `GuardrailResult`.
- Add input/tool/output guardrail stages.
- Represent future handoffs as internal `AgentMessage` events before adopting external protocols.

Reference:
- https://openai.github.io/openai-agents-python/
- https://openai.github.io/openai-agents-python/guardrails/
- https://openai.github.io/openai-agents-python/handoffs/

### CrewAI

Absorb:

- Agent / Task / Crew / Flow separation.
- Task templates.
- Knowledge and memory as first-class context.
- Flow for deterministic business logic and crew for collaborative reasoning.

MemoChat adaptation:

- Keep single-agent default first.
- Add reusable `AgentSpec` templates: researcher, writer, reviewer, support.
- Add crew templates only after trace/replay/guardrails are stable.

Reference:
- https://github.com/crewAIInc/crewAI
- https://docs.crewai.com/

### MCP

Absorb:

- Tool schema and capability discovery.
- Tools, resources, prompts, roots, and elicitation concepts.
- Explicit user confirmation for sensitive operations.
- Tool lifecycle, timeout, stderr/log capture, and result summaries.

MemoChat adaptation:

- Unify built-in tools and MCP tools through `ToolSpec`.
- Add permission levels and `requires_confirmation`.
- Surface tool availability in the QML Agent UI.

Reference:
- https://modelcontextprotocol.io/

### A2A

Absorb later:

- Agent Card style capability metadata.
- Task lifecycle for agent-to-agent delegation.
- Streaming task status and artifacts.
- Reconnect/resubscribe for ongoing tasks.

MemoChat adaptation:

- Do not implement full A2A now.
- Reserve contracts: `AgentCard`, `RemoteAgentPeer`, `RemoteAgentTask`.
- Expose local agent capabilities in a future `/.well-known/agent.json` or internal endpoint.

Reference:
- https://google-a2a.github.io/A2A/specification/

## MemoChat Modules

Server:

- `apps/server/core/AIOrchestrator`
- `apps/server/core/AIServer`
- `apps/server/core/GateServerCore`

Client:

- `apps/client/desktop/MemoChat-qml/qml/agent`
- `apps/client/desktop/MemoChat-qml/AgentController.*`

Data and infrastructure:

- Qdrant: host port `6333/6334`
- Neo4j: host port `7474/7687`
- Ollama: host port `11434`
- AIOrchestrator: host port `8096`
- Postgres: host port `15432`
- Redis: host port `6379`

Observability:

- Grafana `3000`
- Prometheus `9090`
- Loki `3100`
- Tempo `3200`

## Target Architecture

```text
QML Agent Workspace
  Agent chat
  Agent tasks
  Trace tree
  Tool approvals
  Memory viewer
  Eval/debug panel

GateServer / AIServer
  Backward-compatible AI API bridge

AIOrchestrator
  /chat
  /chat/stream
  /agent/run
  /agent/run/stream
  /agent/runs/{trace_id}
  /agent/runs/{trace_id}/graph
  /agent/tasks
  /agent/tools
  /agent/specs
  /agent/evals

Harness
  contracts
  run graph
  agent specs
  tool registry
  guardrails
  context packs
  memory
  knowledge/RAG
  LLM provider registry
  trace/replay/evals
```

## Stage 0: Stabilize Current Agent

Goal:

Make the current AI chat, model registration, streaming, trace panel, and session lifecycle reliable.

Work:

- Keep current `/chat` and `/chat/stream` behavior stable.
- Ensure API provider registration works after Docker rebuild.
- Ensure AI sessions load correctly after login/account switch.
- Ensure trace fields returned by AIOrchestrator map cleanly to QML.

Verification:

```powershell
cmake --preset msvc2022-client-verify
cmake --build --preset msvc2022-client-verify
python -m compileall apps\server\core\AIOrchestrator
docker compose -f apps\server\core\AIOrchestrator\docker-compose.yml up -d --build memochat-ai-orchestrator
Invoke-WebRequest http://127.0.0.1:8096/health -UseBasicParsing
```

## Stage 1: Project-Owned Contracts

Goal:

Create additive contracts that let MemoChat evolve without rewriting old APIs.

Implement in:

- `apps/server/core/AIOrchestrator/harness/contracts.py`

Add:

- `AgentSpec`
- `ToolSpec`
- `GuardrailResult`
- `RunNode`
- `RunGraph`
- `ContextPack`
- `AgentTask`

Rules:

- Do not remove current dataclasses.
- Add `to_dict()` helpers.
- Keep old `AgentTrace.events` response compatible.

Verification:

```powershell
python -m compileall apps\server\core\AIOrchestrator
```

## Stage 2: Trace Tree And RunGraph Snapshot

Goal:

Make every agent run inspectable as a graph, even if execution remains linear at first.

Implement:

- Extend `TraceStore` with graph snapshot storage.
- Convert each current phase into a `RunNode`.
- Add endpoint `GET /agent/runs/{trace_id}/graph`.
- Keep existing `GET /agent/runs/{trace_id}` unchanged.

QML:

- Upgrade trace panel to show node list: plan, memory, tools, model, save, feedback.
- Show status, latency, summary, and safe error.

Verification:

- Run one chat.
- Open trace panel.
- Confirm graph endpoint returns nodes.
- Confirm old trace endpoint still works.

## Stage 3: ToolSpec, Permissions, And Confirmation

Goal:

Make tools trustworthy and visible.

Implement:

- Convert built-in tools to `ToolSpec`.
- Add fields: `parameters_schema`, `timeout_ms`, `permission`, `requires_confirmation`.
- Add parameter validation before execution.
- Add safe tool output summarization.
- Add trace event for denied, skipped, failed, and completed tool calls.

QML:

- Add tool list view.
- Add approval popup for sensitive tools.

Policy:

- Read-only tools can run automatically.
- Write/delete/send/upload/external API tools require confirmation.

Verification:

- `/agent/tools` returns schema and permission metadata.
- A sensitive mock tool cannot execute without approval.

## Stage 4: Guardrails

Goal:

Introduce trust boundaries before MemoChat grows more autonomous.

Implement:

- `harness/guardrails/`
- Input guardrails: empty input, oversized input, obvious unsafe tool intent.
- Tool guardrails: permission, confirmation, argument schema, timeout.
- Output guardrails: empty output, leaked API key pattern, malformed structured output.

Behavior:

- Guardrail pass: continue.
- Guardrail warn: continue and trace warning.
- Guardrail block: stop run with structured error and trace node.

Verification:

- Blocked run returns stable error shape.
- Trace includes guardrail node.

## Stage 5: ContextPack And Visible Memory

Goal:

Move from prompt stuffing to structured context engineering.

Implement:

- `ContextPack`
- Sections: recent chat, session summary, user profile, graph memory, knowledge snippets, tool observations.
- Token budget and source metadata.
- Context provenance in trace.

QML:

- Add memory/context viewer.
- Let users inspect and delete remembered facts.

Data:

- Neo4j for graph memory.
- Postgres or existing trace store for episodic summaries.
- Qdrant for knowledge snippets.

Verification:

- Trace shows which context sections were used.
- User can delete a remembered fact.

## Stage 6: AgentTask And Durable Execution

Goal:

Make AI Agent capable of long-running tasks independent from one chat request.

Implement:

- `AgentTask`
- task states: queued, running, waiting_for_user, completed, failed, cancelled.
- checkpoint after every node.
- resume failed or interrupted runs.
- cancel task endpoint.

APIs:

- `POST /agent/tasks`
- `GET /agent/tasks`
- `GET /agent/tasks/{task_id}`
- `POST /agent/tasks/{task_id}/cancel`
- `POST /agent/tasks/{task_id}/resume`

QML:

- Agent task inbox.
- Task detail view with trace graph.
- Resume/cancel/retry actions.

Verification:

- Start a task.
- Interrupt/restart AIOrchestrator.
- Resume from last checkpoint without redoing completed tool calls.

## Stage 7: Replay And Regression Evals

Goal:

Make harness changes measurable.

Implement:

- `evals/agent_tasks/*.json`
- replay runner with fixed inputs and mocked tools.
- compare expected trace shape, not exact model text.
- metrics: latency, model, tool calls, guardrail result, pass/fail.

APIs:

- `GET /agent/evals`
- `POST /agent/evals/run`

Scripts:

- `tools/scripts/ai/run_agent_evals.ps1`

Verification:

- Minimum eval suite runs locally.
- Failed eval links to trace.

## Stage 8: AgentSpec Templates

Goal:

Move from one generic assistant to configurable specialized agents.

Implement:

- `agents/specs/*.yaml`
- researcher
- writer
- reviewer
- support
- knowledge_curator

Each spec includes:

- instructions
- default model policy
- allowed tools
- memory policy
- knowledge policy
- guardrails

QML:

- Agent mode selector.
- Show what each agent can access.

Verification:

- Selecting an agent changes tools/model policy without changing core API.

## Stage 9: Multi-Agent And Handoffs

Goal:

Add controlled collaboration after single-agent reliability is strong.

Implement:

- Internal `AgentMessage`.
- Handoff node in RunGraph.
- `CrewSpec` or `FlowSpec`.
- Handoff trace spans.

Initial crews:

- research -> write -> review
- support triage -> KB lookup -> answer
- document ingest -> summarize -> memory write

Verification:

- One multi-agent run produces a single trace graph with child agent nodes.

## Stage 10: A2A-Ready Interop

Goal:

Prepare MemoChat for external agent ecosystems without overcommitting early.

Implement:

- `AgentCard` contract.
- endpoint for local capabilities.
- remote agent registry placeholder.
- remote task contract shape aligned with A2A concepts.

Do not:

- Expose unsafe remote execution by default.
- Let remote agents access MemoChat memory without explicit permission.

Verification:

- Local agent card describes capabilities, auth requirement, skills, and endpoints.

## Build And Runtime Commands

Client:

```powershell
cmake --preset msvc2022-client-verify
cmake --build --preset msvc2022-client-verify
```

Server:

```powershell
cmake --preset msvc2022-server-verify
cmake --build --preset msvc2022-server-verify
```

AIOrchestrator:

```powershell
python -m compileall apps\server\core\AIOrchestrator
docker compose -f apps\server\core\AIOrchestrator\docker-compose.yml up -d --build memochat-ai-orchestrator
Invoke-WebRequest http://127.0.0.1:8096/health -UseBasicParsing
```

Docker checks:

```powershell
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
Invoke-WebRequest http://127.0.0.1:11434/api/tags -UseBasicParsing
Invoke-WebRequest http://127.0.0.1:6333/ -UseBasicParsing
Invoke-WebRequest http://127.0.0.1:7474/ -UseBasicParsing
```

## Failure Handling

- If C++ deploy fails with access denied, stop old service processes before retrying.
- If Python AIOrchestrator route is missing after code changes, rebuild the Docker image.
- If model provider registration fails, check `/models/api-provider`, provider base URL, and Docker logs.
- Never log API keys or full provider credentials.
- Keep all infrastructure in Docker and preserve fixed ports.

## Commit Guidance

- Do not include `.ai/` planning artifacts in normal feature commits unless the user explicitly asks.
- Commit code/docs changes separately from runtime data and generated files.
