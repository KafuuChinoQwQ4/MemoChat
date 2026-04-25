from __future__ import annotations

from tools.registry import ToolRegistry


class MCPToolService:
    def list_tools(self) -> list[dict]:
        registry = ToolRegistry.get_instance()
        return [
            {"name": tool.name, "description": tool.description or ""}
            for tool in registry.get_tools()
            if tool.name.startswith("mcp_")
        ]
