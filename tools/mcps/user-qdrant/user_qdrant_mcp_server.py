#!/usr/bin/env python3
"""
user-qdrant MCP Server
通过 stdio + JSON-RPC 2.0 与 Cursor 通信，代理 Qdrant 向量数据库操作。

依赖安装: pip install qdrant-client
"""
import sys
import json
from typing import Any

try:
    from qdrant_client import QdrantClient
    from qdrant_client.http import models
except ImportError:
    print("请安装 qdrant-client: pip install qdrant-client", file=sys.stderr)
    sys.exit(1)


QDRANT_HOST = "127.0.0.1"
QDRANT_PORT = 6333

_client = None


def get_client() -> QdrantClient:
    global _client
    if _client is None:
        _client = QdrantClient(host=QDRANT_HOST, port=QDRANT_PORT, timeout=30)
    return _client


TOOLS = {
    "qdrant_list_collections": {
        "name": "qdrant_list_collections",
        "description": "List all collections in the Qdrant vector database.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "qdrant_search": {
        "name": "qdrant_search",
        "description": "Search for similar vectors in a Qdrant collection by cosine similarity.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "collection_name": {"type": "string"},
                "query_vector": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": "Query vector (embedding) as list of floats.",
                },
                "limit": {"type": "number", "default": 5},
                "score_threshold": {"type": "number", "default": 0.7},
            },
            "required": ["collection_name", "query_vector"],
        },
    },
    "qdrant_upsert": {
        "name": "qdrant_upsert",
        "description": "Insert or update points (vectors with payload) in a Qdrant collection.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "collection_name": {"type": "string"},
                "points": {
                    "type": "array",
                    "description": "List of points with 'id', 'vector', and 'payload'.",
                },
            },
            "required": ["collection_name", "points"],
        },
    },
    "qdrant_delete_collection": {
        "name": "qdrant_delete_collection",
        "description": "Delete a collection and all its points from Qdrant.",
        "inputSchema": {
            "type": "object",
            "properties": {"collection_name": {"type": "string"}},
            "required": ["collection_name"],
        },
    },
    "qdrant_scroll": {
        "name": "qdrant_scroll",
        "description": "Scroll through all points in a collection (paginated listing).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "collection_name": {"type": "string"},
                "limit": {"type": "number", "default": 100},
                "offset": {"type": "string"},
                "with_payload": {"type": "boolean", "default": True},
            },
            "required": ["collection_name"],
        },
    },
}


TOOL_HANDLERS = {}


def _init_handlers():
    client = get_client()

    def list_collections(_):
        collections = client.get_collections().collections
        return {
            "collections": [
                {
                    "name": c.name,
                    "vectors_count": client.get_collection(c.name).vectors_count,
                    "status": client.get_collection(c.name).status,
                }
                for c in collections
            ]
        }

    def search(args):
        limit = args.get("limit", 5)
        threshold = args.get("score_threshold", 0.7)
        results = client.search(
            collection_name=args["collection_name"],
            query_vector=args["query_vector"],
            limit=limit,
            score_threshold=threshold,
        )
        return {
            "results": [
                {
                    "id": str(hit.id),
                    "score": hit.score,
                    "payload": hit.payload,
                }
                for hit in results
            ]
        }

    def upsert(args):
        points = args["points"]
        client.upsert(
            collection_name=args["collection_name"],
            points=points,
        )
        return {"status": "success", "upserted": len(points)}

    def delete_collection(args):
        client.delete_collection(collection_name=args["collection_name"])
        return {"status": "success", "deleted": args["collection_name"]}

    def scroll(args):
        offset = args.get("offset")
        results = client.scroll(
            collection_name=args["collection_name"],
            limit=args.get("limit", 100),
            offset=offset,
            with_payload=args.get("with_payload", True),
        )
        return {
            "points": [
                {"id": str(p.id), "vector": p.vector, "payload": p.payload}
                for p in results[0]
            ],
            "next_page_offset": str(results[1]) if results[1] else None,
        }

    TOOL_HANDLERS["qdrant_list_collections"] = list_collections
    TOOL_HANDLERS["qdrant_search"] = search
    TOOL_HANDLERS["qdrant_upsert"] = upsert
    TOOL_HANDLERS["qdrant_delete_collection"] = delete_collection
    TOOL_HANDLERS["qdrant_scroll"] = scroll


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

    async def handle_message(self, msg: dict) -> None:
        method = msg.get("method")
        msg_id = msg.get("id")
        params = msg.get("params", {})

        if method == "initialize":
            resp = {
                "protocolVersion": "2024-11-05",
                "capabilities": {"tools": {}},
                "serverInfo": {"name": "user-qdrant", "version": "1.0.0"},
            }
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": resp})
            self._initialized = True

        elif method == "notifications/initialized":
            pass

        elif method == "tools/list":
            tools_list = [
                {
                    "name": t["name"],
                    "description": t["description"],
                    "inputSchema": t["inputSchema"],
                }
                for t in TOOLS.values()
            ]
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": {"tools": tools_list}})

        elif method == "tools/call":
            tool_name = params.get("name")
            tool_args = params.get("arguments", {})
            try:
                if tool_name not in TOOL_HANDLERS:
                    raise ValueError(f"Unknown tool: {tool_name}")
                result = TOOL_HANDLERS[tool_name](tool_args)
                text = json.dumps(result, ensure_ascii=False, indent=2)
                self._send({
                    "jsonrpc": "2.0",
                    "id": msg_id,
                    "result": {"content": [{"type": "text", "text": text}]},
                })
            except Exception as e:
                self._send({
                    "jsonrpc": "2.0",
                    "id": msg_id,
                    "result": {
                        "content": [
                            {"type": "text", "text": f"Qdrant 错误: {type(e).__name__}: {e}"}
                        ]
                    },
                })

        elif method == "shutdown":
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": None})
            sys.exit(0)

    def run(self) -> None:
        _init_handlers()
        while True:
            msg = self._recv()
            if msg is None:
                break
            # handle_message is sync-safe for most tools, run in sync mode
            import asyncio
            asyncio.run(self.handle_message(msg))


def main():
    server = MCPStdioServer()
    server.run()


if __name__ == "__main__":
    main()
