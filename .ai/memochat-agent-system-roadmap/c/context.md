# Context

Task:
Continue MemoChat AI Agent roadmap with `stage-3-tools-permissions`.

Relevant files:
- `apps/server/core/AIOrchestrator/harness/contracts.py`
- `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`
- `apps/server/core/AIOrchestrator/harness/ports.py`
- `apps/server/core/AIOrchestrator/api/agent_router.py`
- `apps/server/core/AIOrchestrator/schemas/api.py`
- `apps/server/core/AIOrchestrator/tools/registry.py`
- `apps/server/core/AIOrchestrator/tests/test_harness_structure.py`
- `.ai/memochat-agent-system-roadmap/tasks.json`

Current state:
- Stage 1 added `ToolSpec`.
- Stage 2 added `RunGraph` projection and graph endpoint.
- `ToolExecutor.list_tools()` currently returns only `name`, `description`, and `source`.
- Explicit requested tools are executed under the `mcp_tool` plan action without name validation, schema validation, or confirmation checks.
- Existing auto-planned built-in paths include knowledge search, web search, calculator, graph recall, and MCP explicit calls.

Implementation decision:
- Add a compatibility-preserving `ToolSpec` projection over existing LangChain tools.
- Return schema, timeout, permission, and confirmation metadata from `/agent/tools`.
- Add additive request field `confirmed_tools`.
- Enforce unknown tool denial and confirmation only for explicitly requested tools.
- Keep existing planner-driven built-in tool paths working while the QML approval UI is added later.

Infrastructure:
- No Docker data mutation is needed.
- No stable Docker ports or runtime service configs are changed.

Verification target:
- Python compile for touched AIOrchestrator modules.
- `python -m unittest tests.test_harness_structure` in the AIOrchestrator `.venv`.

Open risks:
- Full MCP argument schema fidelity depends on metadata exposed by LangChain/MCP tool wrappers.
- Client approval UI is not part of this stage, so confirmation is only represented through additive API fields.
