#!/usr/bin/env python3
"""
user-neo4j MCP Server
通过 stdio + JSON-RPC 2.0 与 Cursor 通信，代理 Neo4j 图数据库操作。

依赖安装: pip install neo4j pydantic
"""
import sys
import json
import asyncio
from typing import Any

try:
    from neo4j import AsyncGraphDatabase
except ImportError:
    print("请安装 neo4j 驱动: pip install neo4j", file=sys.stderr)
    sys.exit(1)


NEO4J_URI = "bolt://127.0.0.1:7687"
NEO4J_USER = "neo4j"
NEO4J_PASSWORD = "password"
NEO4J_DATABASE = "neo4j"

_driver = None


async def get_driver():
    global _driver
    if _driver is None:
        _driver = AsyncGraphDatabase.driver(
            NEO4J_URI,
            auth=(NEO4J_USER, NEO4J_PASSWORD),
            max_connection_lifetime=3600,
        )
    return _driver


async def close_driver():
    global _driver
    if _driver:
        try:
            await _driver.close()
        except Exception:
            pass
        _driver = None


async def run_cypher(query: str, params: dict | None = None) -> list[dict]:
    driver = await get_driver()
    params = params or {}
    async with driver.session(database=NEO4J_DATABASE) as session:
        result = await session.run(query, **params)
        records = await result.data()
        return records


async def get_schema() -> dict:
    """获取 Neo4j 数据库 schema"""
    labels_result = await run_cypher("CALL db.labels() YIELD label RETURN label")
    rel_types_result = await run_cypher("CALL db.relationshipTypes() YIELD relationshipType RETURN relationshipType")
    props_result = await run_cypher(
        "MATCH (n) UNWIND keys(n) as k WITH DISTINCT k RETURN k as propertyKey LIMIT 100"
    )
    return {
        "labels": [r["label"] for r in labels_result],
        "relationship_types": [r["relationshipType"] for r in rel_types_result],
        "node_properties": [r["propertyKey"] for r in props_result],
    }


TOOLS = {
    "read_neo4j_cypher": {
        "name": "read_neo4j_cypher",
        "description": "Execute a read Cypher query on the Neo4j graph database. "
                      "Use this for MATCH, RETURN, WITH, WHERE, ORDER BY, LIMIT, etc. "
                      "All queries are read-only (MATCH/OPTIONAL MATCH only).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "The Cypher query to execute. Use only read operations (MATCH, OPTIONAL MATCH).",
                },
                "params": {
                    "type": "object",
                    "default": {},
                    "description": "Query parameters as key-value pairs.",
                },
            },
            "required": ["query"],
        },
    },
    "write_neo4j_cypher": {
        "name": "write_neo4j_cypher",
        "description": "Execute a write Cypher query on the Neo4j graph database. "
                      "Use for CREATE, MERGE, SET, DELETE, DETACH DELETE, REMOVE, etc.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "The Cypher query to execute. Can include write operations.",
                },
                "params": {
                    "type": "object",
                    "default": {},
                    "description": "Query parameters as key-value pairs.",
                },
            },
            "required": ["query"],
        },
    },
    "get_neo4j_schema": {
        "name": "get_neo4j_schema",
        "description": "Get the schema of the Neo4j database: labels, relationship types, and node properties.",
        "inputSchema": {"type": "object", "properties": {}},
    },
}

TOOL_HANDLERS = {
    "read_neo4j_cypher": lambda args: run_cypher(args["query"], args.get("params")),
    "write_neo4j_cypher": lambda args: run_cypher(args["query"], args.get("params")),
    "get_neo4j_schema": lambda _: get_schema(),
}

READ_ONLY_PATTERNS = (
    "MATCH", "OPTIONAL MATCH", "CALL", "YIELD",
    "WHERE", "RETURN", "WITH", "ORDER BY", "LIMIT",
    "SKIP", "DISTINCT", "COUNT", "COLLECT", "REDUCE",
    "HEAD", "TAIL", "LAST", "START", "LOAD CSV",
)


class MCPStdioServer:
    def __init__(self):
        self._initialized = False
        self._request_id = 0

    def _next_id(self) -> int:
        self._request_id += 1
        return self._request_id

    def _send(self, msg: dict) -> None:
        print(json.dumps(msg), flush=True)

    def _recv(self) -> dict | None:
        try:
            line = sys.stdin.readline()
            if not line:
                return None
            return json.loads(line.lstrip("\ufeff"))
        except (json.JSONDecodeError, EOFError):
            return None

    async def _handle_initialize(self, params: dict) -> dict:
        return {
            "protocolVersion": "2024-11-05",
            "capabilities": {"tools": {}},
            "serverInfo": {"name": "user-neo4j", "version": "1.0.0"},
        }

    async def _handle_tools_list(self) -> dict:
        tools_list = [
            {
                "name": t["name"],
                "description": t["description"],
                "inputSchema": t["inputSchema"],
            }
            for t in TOOLS.values()
        ]
        return {"tools": tools_list}

    async def _handle_tools_call(self, params: dict) -> dict:
        tool_name = params.get("name")
        tool_args = params.get("arguments", {})

        if tool_name not in TOOL_HANDLERS:
            raise ValueError(f"Unknown tool: {tool_name}")

        # 安全检查：只读工具不能包含写操作关键字
        if tool_name == "read_neo4j_cypher":
            query_upper = tool_args.get("query", "").upper()
            if any(p in query_upper for p in ("CREATE ", "MERGE ", "SET ", "DELETE ", "REMOVE ", "DROP ", "DETACH")):
                return {
                    "content": [
                        {
                            "type": "text",
                            "text": "安全拒绝：read_neo4j_cypher 工具仅支持只读查询。"
                                    "请使用 write_neo4j_cypher 执行写入操作。",
                        }
                    ]
                }

        try:
            result = await TOOL_HANDLERS[tool_name](tool_args)
            if isinstance(result, list):
                text = json.dumps(result, ensure_ascii=False, indent=2)
            else:
                text = json.dumps(result, ensure_ascii=False, indent=2)
            return {
                "content": [{"type": "text", "text": text}]
            }
        except Exception as e:
            return {
                "content": [{"type": "text", "text": f"Neo4j 查询错误: {type(e).__name__}: {e}"}]
            }

    async def handle_message(self, msg: dict) -> None:
        method = msg.get("method")
        msg_id = msg.get("id")
        params = msg.get("params", {})

        if method == "initialize":
            resp = await self._handle_initialize(params)
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": resp})
            self._initialized = True

        elif method == "notifications/initialized":
            pass

        elif method == "tools/list":
            resp = await self._handle_tools_list()
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": resp})

        elif method == "tools/call":
            try:
                result = await self._handle_tools_call(params)
                self._send({"jsonrpc": "2.0", "id": msg_id, "result": result})
            except Exception as e:
                self._send({
                    "jsonrpc": "2.0",
                    "id": msg_id,
                    "error": {"code": -32603, "message": str(e)},
                })

        elif method == "shutdown":
            await close_driver()
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": None})
            sys.exit(0)

    async def run(self) -> None:
        while True:
            msg = self._recv()
            if msg is None:
                break
            await self.handle_message(msg)


async def main_async():
    server = MCPStdioServer()
    try:
        await server.run()
    finally:
        await close_driver()


def main():
    asyncio.run(main_async())


if __name__ == "__main__":
    main()
