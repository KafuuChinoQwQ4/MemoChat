# AI Agent Harness

This directory is the production AI Agent boundary for MemoChat.

Layer map:
- `orchestration/`: resolves skills, builds plans, and coordinates one agent turn.
- `execution/`: executes plan actions through tools, RAG, graph recall, and MCP-capable registries.
- `feedback/`: records trace events, run outcomes, and user feedback.
- `memory/`: loads and writes short-term, episodic, semantic, and graph memory.
- `knowledge/`: owns knowledge-base upload/search/delete workflows.
- `llm/`: resolves local/external model endpoints and calls models.
- `mcp/`: exposes MCP tool metadata through the harness boundary.
- `skills/`: owns skill definitions and intent-to-skill resolution.
- `runtime/`: composes concrete implementations.

Dependency direction:
- API routes call `runtime.HarnessContainer`.
- `runtime` wires concrete services.
- `orchestration` depends on ports from `harness/ports.py`, not concrete layer implementations.
- Domain contracts live in `harness/contracts.py`.
- Layer discovery metadata lives in `harness/layers.py` and is exposed by `GET /agent/layers`.
