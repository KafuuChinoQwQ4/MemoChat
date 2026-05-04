from __future__ import annotations

from tools.registry import ToolRegistry


class MCPToolService:
    def list_tools(self) -> list[dict]:
        registry = ToolRegistry.get_instance()
        from harness.execution.tool_executor import ToolExecutor

        executor = ToolExecutor.__new__(ToolExecutor)
        return [
            spec.to_dict()
            for spec in (executor._tool_to_spec(tool) for tool in registry.get_tools())
            if spec.name.startswith("mcp_")
        ]
