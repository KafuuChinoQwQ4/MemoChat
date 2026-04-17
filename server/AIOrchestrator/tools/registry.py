"""
工具注册表 — 注册所有可用工具（含 MCP）
"""
import asyncio
import structlog
from typing import Optional
from langchain_core.tools import BaseTool

from .duckduckgo_tool import DuckDuckGoSearchTool
from .knowledge_base_tool import KnowledgeBaseTool
from .calculator_tool import CalculatorTool
from .translator_tool import TranslatorTool
from .mcp_bridge import MCPBridge

logger = structlog.get_logger()


class ToolRegistry:
    """
    LangChain 工具注册表（单例）
    
    注册内置工具和 MCP 工具，提供统一的工具查询接口。
    """
    _instance: Optional["ToolRegistry"] = None

    def __init__(self):
        self._tools: list[BaseTool] = []
        self._initialized = False
        self._mcp_bridge: MCPBridge | None = None

    @classmethod
    def get_instance(cls) -> "ToolRegistry":
        if cls._instance is None:
            cls._instance = ToolRegistry()
            cls._instance._init_tools()
        return cls._instance

    def _init_tools(self) -> None:
        """初始化内置工具"""
        if self._initialized:
            return

        tool_instances = [
            DuckDuckGoSearchTool(),
            KnowledgeBaseTool(),
            CalculatorTool(),
            TranslatorTool(),
        ]

        for inst in tool_instances:
            if hasattr(inst, "get_tool"):
                t = inst.get_tool()
                self._tools.append(t)
                logger.info("tool.registered", name=t.name)

        self._initialized = True

    async def initialize_mcp(self) -> None:
        """
        异步初始化 MCP 桥接层。
        需在 FastAPI 启动事件中调用（事件循环已就绪）。
        """
        if self._mcp_bridge is not None:
            return  # 已初始化

        try:
            from config import settings
            mcp_config = getattr(settings, 'mcp', None)
            if mcp_config is None:
                logger.info("mcp.not_configured")
                return

            # 支持 Pydantic 模型或 dict
            if hasattr(mcp_config, 'model_dump'):
                mcp_dict = mcp_config.model_dump()
            elif isinstance(mcp_config, dict):
                mcp_dict = mcp_config
            else:
                mcp_dict = {}
                logger.warning("mcp.config.unknown_format", type=type(mcp_config).__name__)
                return

            self._mcp_bridge = MCPBridge(mcp_dict)
            if self._mcp_bridge.enabled:
                await self._mcp_bridge.initialize()
                self._tools.extend(self._mcp_bridge.get_tools())
                logger.info("mcp.tools.integrated", count=len(self._mcp_bridge.get_tools()))
            else:
                logger.info("mcp.disabled_by_config")

        except ImportError as e:
            logger.warning("mcp.import.error", error=str(e))
        except Exception as e:
            logger.error("mcp.init.error", error=str(e))

    def get_tools(self) -> list[BaseTool]:
        """返回所有已注册工具"""
        return self._tools

    def get_tool_names(self) -> list[str]:
        """返回所有工具名称"""
        return [t.name for t in self._tools]

    def get_tools_schema(self) -> list[dict]:
        """返回所有工具的 schema（用于动态生成 system prompt）"""
        schema = [
            {"name": t.name, "description": t.description}
            for t in self._tools
            if not t.name.startswith("mcp_")  # MCP 工具从 bridge 获取
        ]
        if self._mcp_bridge:
            schema.extend(self._mcp_bridge.get_tools_schema())
        return schema

    async def close(self) -> None:
        """关闭所有工具连接（含 MCP Server）"""
        if self._mcp_bridge:
            await self._mcp_bridge.close()
