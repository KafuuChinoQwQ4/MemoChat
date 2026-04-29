# Execution Layer

Purpose:
- Execute planned actions such as knowledge search, web search, calculation, graph recall, and explicit tool calls.
- Convert external tool outputs into `ToolObservation` values for orchestration.

Primary files:
- `tool_executor.py`

Coupling rule:
- Tool-specific failures should become observations instead of breaking the whole run unless the caller explicitly requires hard failure.
