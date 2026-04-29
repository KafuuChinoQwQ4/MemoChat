# MCP Layer

Purpose:
- Surface MCP-capable tools registered in the shared `ToolRegistry`.
- Keep MCP metadata queryable through the harness boundary.

Primary files:
- `service.py`

Coupling rule:
- Process lifecycle and tool discovery remain in the lower registry; harness code only exposes safe metadata and execution entry points.
