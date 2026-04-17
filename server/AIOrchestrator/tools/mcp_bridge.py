"""
MCP (Model Context Protocol) 桥接层

支持通过 subprocess + stdio + JSON-RPC 2.0 与外部 MCP Server 通信。
MCP 协议标准参考: https://modelcontextprotocol.io

工作流程：
1. 启动 MCP Server subprocess
2. 发送 initialize 请求建立连接
3. 调用 tools/list 获取可用工具列表
4. 通过 tools/call 调用具体工具
"""
import asyncio
import json
import subprocess
import os
import structlog
from typing import Any, Optional
from dataclasses import dataclass, field
from langchain_core.tools import BaseTool, tool

logger = structlog.get_logger()


@dataclass
class MCPServerConfig:
    """MCP Server 配置"""
    name: str
    command: list[str]
    args: list[str] = field(default_factory=list)
    env: dict[str, str] = field(default_factory=dict)
    enabled: bool = True


class MCPBridge:
    """
    MCP 桥接层

    支持直接通过 subprocess 启动 MCP Server 并通过 stdio 通信。
    MCP 协议基于 JSON-RPC 2.0。
    """

    def __init__(self, config: dict | None = None):
        self._config = config or {}
        self._enabled = self._config.get("enabled", False)
        self._servers: list[MCPServerConfig] = []
        for srv in self._config.get("servers", []):
            if srv.get("enabled", False):
                self._servers.append(MCPServerConfig(**srv))
        self._initialized = False
        self._tools: list[BaseTool] = []
        self._server_processes: list[asyncio.subprocess.Process] = []
        self._call_id_counter = 0

    @property
    def enabled(self) -> bool:
        return self._enabled and len(self._servers) > 0

    async def initialize(self) -> None:
        """连接所有启用的 MCP Server 并注册工具"""
        if not self.enabled or self._initialized:
            return

        logger.info("mcp.init", servers=len(self._servers))

        for srv_cfg in self._servers:
            try:
                await self._connect_server(srv_cfg)
            except Exception as e:
                logger.warning("mcp.server.skip",
                             name=srv_cfg.name,
                             error=str(e),
                             command=" ".join(srv_cfg.command))

        self._initialized = True
        logger.info("mcp.ready", tools=len(self._tools),
                   tool_names=[t.name for t in self._tools])

    async def _connect_server(self, srv_cfg: MCPServerConfig) -> None:
        """启动 MCP Server subprocess 并初始化"""
        logger.info("mcp.connecting", name=srv_cfg.name, cmd=srv_cfg.command)

        env = {**os.environ, **srv_cfg.env}
        full_cmd = srv_cfg.command + srv_cfg.args

        proc = await asyncio.create_subprocess_exec(
            *full_cmd,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            env=env,
        )
        self._server_processes.append(proc)

        try:
            # 1. 发送 initialize
            init_req = {
                "jsonrpc": "2.0",
                "id": self._next_id(),
                "method": "initialize",
                "params": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {"roots": {"listChanged": True}},
                    "clientInfo": {"name": "memochat-ai-orchestrator", "version": "1.0.0"},
                },
            }
            await self._send(proc, init_req)

            # 2. 读取 initialize 响应
            init_resp = await self._recv(proc, timeout=15)
            if not init_resp or "error" in init_resp:
                error_msg = init_resp.get("error", {}) if init_resp else "no response"
                raise RuntimeError(f"Initialize failed: {error_msg}")

            # 3. 发送 initialized notification
            notif = {"jsonrpc": "2.0", "method": "notifications/initialized", "params": {}}
            await self._send(proc, notif)

            # 4. 调用 tools/list
            list_req = {
                "jsonrpc": "2.0",
                "id": self._next_id(),
                "method": "tools/list",
                "params": {},
            }
            await self._send(proc, list_req)

            # 5. 读取 tools/list 响应
            list_resp = await self._recv(proc, timeout=15)
            tools_meta = []
            if list_resp and "result" in list_resp:
                tools_meta = list_resp["result"].get("tools", [])

            # 6. 为每个工具创建 LangChain wrapper
            for t_meta in tools_meta:
                tool_inst = self._mcp_tool_to_langchain(t_meta, srv_cfg.name, proc)
                self._tools.append(tool_inst)
                logger.info("mcp.tool.registered",
                          server=srv_cfg.name,
                          tool=t_meta.get("name"),
                          description=t_meta.get("description", "")[:60])

            logger.info("mcp.server.ready",
                      name=srv_cfg.name,
                      tools=len(tools_meta))

        except Exception as e:
            logger.error("mcp.server.error", name=srv_cfg.name, error=str(e))
            if proc.returncode is None:
                proc.terminate()
            raise

    def _next_id(self) -> int:
        self._call_id_counter += 1
        return self._call_id_counter

    async def _send(self, proc: asyncio.subprocess.Process, msg: dict) -> None:
        """发送 JSON-RPC 消息"""
        data = json.dumps(msg) + "\n"
        proc.stdin.write(data.encode("utf-8"))
        await proc.stdin.drain()

    async def _recv(self, proc: asyncio.subprocess.Process,
                   timeout: float = 30) -> dict | None:
        """接收 JSON-RPC 响应"""
        try:
            line = await asyncio.wait_for(proc.stdout.readline(), timeout=timeout)
            if not line:
                return None
            return json.loads(line.decode("utf-8"))
        except asyncio.TimeoutError:
            logger.warning("mcp.recv.timeout", timeout=timeout)
            return None
        except json.JSONDecodeError as e:
            logger.error("mcp.json.error", error=str(e))
            return None

    def _mcp_tool_to_langchain(self, mcp_tool: dict, server_name: str,
                                proc: asyncio.subprocess.Process) -> BaseTool:
        """将 MCP 工具元数据转换为 LangChain @tool"""
        tool_name = mcp_tool.get("name", "")
        description = mcp_tool.get("description", f"MCP tool from {server_name}")
        input_schema = mcp_tool.get("inputSchema", {})
        properties = input_schema.get("properties", {}) if isinstance(input_schema, dict) else {}

        base_id = self._call_id_counter + 1

        @tool(f"mcp_{server_name}_{tool_name}", description=description)
        async def mcp_tool_wrapper(**kwargs) -> str:
            call_id = self._next_id()
            req = {
                "jsonrpc": "2.0",
                "id": call_id,
                "method": "tools/call",
                "params": {
                    "name": tool_name,
                    "arguments": kwargs,
                },
            }
            try:
                await self._send(proc, req)

                # 读取调用结果
                resp = await self._recv(proc, timeout=60)
                if resp is None:
                    return f"工具 {tool_name} 调用超时（60s）"

                if "error" in resp:
                    err = resp["error"]
                    return f"工具调用失败: {err.get('message', err)}"

                result = resp.get("result", {})
                content = result.get("content", [])
                if content and isinstance(content, list):
                    text_parts = []
                    for c in content:
                        if isinstance(c, dict):
                            text_parts.append(c.get("text", str(c)))
                        else:
                            text_parts.append(str(c))
                    return "\n".join(text_parts)
                return json.dumps(result, ensure_ascii=False, indent=2)

            except BrokenPipeError:
                return f"MCP Server {server_name} 已断开连接"
            except Exception as e:
                return f"工具调用异常: {str(e)}"

        return mcp_tool_wrapper

    def get_tools(self) -> list[BaseTool]:
        """返回所有已注册的 MCP 工具"""
        return self._tools

    def get_tools_schema(self) -> list[dict]:
        """返回所有 MCP 工具的 schema（用于动态生成 system prompt）"""
        return [
            {
                "name": t.name,
                "description": t.description,
            }
            for t in self._tools
        ]

    async def close(self) -> None:
        """关闭所有 MCP Server 进程"""
        logger.info("mcp.closing", processes=len(self._server_processes))
        for proc in self._server_processes:
            if proc.returncode is None:
                try:
                    proc.terminate()
                    await asyncio.wait_for(proc.wait(), timeout=5)
                except asyncio.TimeoutError:
                    proc.kill()
        self._server_processes.clear()
        self._initialized = False
        logger.info("mcp.closed")
