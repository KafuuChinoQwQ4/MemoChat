# MemoChat AI Agent System

MemoChat's AI Agent is evolving from a chat endpoint into a production agent system with explicit contracts, traceable runs, tool permissions, guardrails, structured context, durable tasks, replayable evals, and future agent-to-agent interoperability.

The system stays project-owned. It absorbs proven patterns from LangGraph, OpenAI Agents SDK, CrewAI, MCP, and A2A, but keeps MemoChat's existing QML client, AIServer bridge, AIOrchestrator, Docker services, and local data stores as the source of truth.

Current milestone:
- Stage 0 stabilized the current chat/provider/session behavior.
- Stage 1 added additive Python contracts in `AIOrchestrator/harness/contracts.py`.
- Stage 2 projects current linear traces into `RunGraph` snapshots and exposes graph inspection without breaking the existing trace endpoint.
- Stage 3 exposes structured tool specs, schema validation, permission levels, and confirmation checks.
- Stage 4 adds trace-visible guardrails for input, tool, and output stages.
- Stage 5 adds structured `ContextPack` memory and desktop-visible memory management through Gate/AIServer.
- Stage 6 adds the first durable `AgentTask` lifecycle with JSON-file persistence, checkpoints, cancel/resume endpoints, Gate/AIServer bridge, and a QML task inbox.
- Stage 7 adds replay and regression evals through `harness/evals`, `/agent/evals`, and `/agent/evals/run`, checking status, trace layers, events, tool calls, guardrails, and latency without depending on exact model wording.
- Stage 8 adds reusable `AgentSpec` templates for researcher, writer, reviewer, support, and knowledge curator through `harness/skills/specs.py`, `/agent/specs`, and planner-aware skill resolution.
- Stage 9 adds controlled multi-agent handoffs through internal `AgentMessage`, `HandoffStep`, and `AgentFlow` contracts, `harness/handoffs`, and `/agent/flows` APIs.
- Stage 10 adds A2A-ready interoperability metadata through `AgentCard`, `AgentCardSkill`, `AgentCapabilities`, `RemoteAgentRef`, `harness/interop`, `/agent/card`, and `/agent/remote-agents`.
- The initial AI Agent roadmap is complete through Stage 10. Further work should harden runtime persistence, client surfaces, remote trust policy, and Docker deployment verification.

Infrastructure remains Docker-backed with stable ports. Runtime changes should be verified through the narrowest relevant compile, build, Docker, and MCP checks.
