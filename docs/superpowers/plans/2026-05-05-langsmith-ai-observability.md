# LangSmith AI Observability Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the AIOrchestrator Langfuse-style observability stub with a LangSmith-centered AI observability system.

**Architecture:** Add a focused `observability/langsmith_instrument.py` adapter that owns environment setup, redaction, trace wrappers, and safe no-op behavior. Integrate it into FastAPI startup, Agent run root spans, RAG operations, LLM manager fallback spans, and tool execution spans while preserving Prometheus metrics and local TraceStore.

**Tech Stack:** Python, FastAPI, LangSmith SDK, LangChain tracing conventions, Pydantic config, pytest/unittest, Docker-backed AIOrchestrator runtime.

---

### Task 1: Config And Safe LangSmith Adapter

**Files:**
- Modify: `apps/server/core/AIOrchestrator/config.py`
- Create: `apps/server/core/AIOrchestrator/observability/langsmith_instrument.py`
- Test: `apps/server/core/AIOrchestrator/tests/test_langsmith_observability.py`

- [x] Write tests for config defaults, env setup, redaction, and no-op behavior.
- [x] Run tests and verify they fail before implementation.
- [x] Implement `LangSmithConfig` and safe adapter.
- [x] Run tests and verify pass.

### Task 2: Startup Integration

**Files:**
- Modify: `apps/server/core/AIOrchestrator/main.py`
- Delete or stop importing: `apps/server/core/AIOrchestrator/observability/langfuse_instrument.py`
- Modify: `apps/server/core/AIOrchestrator/config.yaml`
- Modify: `apps/server/core/AIOrchestrator/requirements.txt`

- [x] Write/extend tests to import app initialization without LangSmith credentials.
- [x] Replace `init_langfuse()` with `init_langsmith()`.
- [x] Add `langsmith>=0.2.0` dependency.
- [x] Run compile/test checks.

### Task 3: Agent/RAG/LLM/Tool Instrumentation

**Files:**
- Modify: `apps/server/core/AIOrchestrator/api/agent_router.py`
- Modify: `apps/server/core/AIOrchestrator/api/kb_router.py`
- Modify: `apps/server/core/AIOrchestrator/harness/orchestration/agent_service.py`
- Modify: `apps/server/core/AIOrchestrator/harness/execution/tool_executor.py`
- Modify: `apps/server/core/AIOrchestrator/rag/chain.py`
- Modify: `apps/server/core/AIOrchestrator/llm/manager.py`
- Modify: `apps/server/core/AIOrchestrator/llm/ollama_llm.py`

- [x] Add root trace wrappers for `/agent/run` and `/kb/*`.
- [x] Add child spans for planning, guardrails, memory, tool execution, RAG add/retrieve, LLM fallback attempts, and Ollama model call.
- [x] Keep local TraceStore event semantics unchanged.
- [x] Verify no import-time failure if `langsmith` package is missing.

### Task 4: Docs And Verification

**Files:**
- Create: `docs/ai-agent-langsmith-observability.md`
- Modify as needed: `docs/ai_agent_e2e_test.md`

- [x] Document LangSmith env vars, privacy defaults, trace taxonomy, and eval strategy.
- [x] Run `python -m compileall apps/server/core/AIOrchestrator`.
- [x] Run focused tests.
- [x] Record verification output under `.ai/langsmith-ai-observability/a/logs/phase-verify.result.md`.
