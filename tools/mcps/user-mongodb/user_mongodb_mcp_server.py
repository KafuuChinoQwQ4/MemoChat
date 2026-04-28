#!/usr/bin/env python3
import json
import subprocess
import sys
from typing import Any

CONTAINER = "memochat-mongo"
MONGO_URI = "mongodb://memochat_app:123456@127.0.0.1:27017/memochat"
TIMEOUT_SECONDS = 30

TOOLS = {
    "mongodb_list_collections": {
        "name": "mongodb_list_collections",
        "description": "List MongoDB collections in the memochat database.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "mongodb_count_documents": {
        "name": "mongodb_count_documents",
        "description": "Count documents in a MongoDB collection.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "collection": {"type": "string"},
                "filter": {"type": "object", "default": {}},
            },
            "required": ["collection"],
        },
    },
    "mongodb_find": {
        "name": "mongodb_find",
        "description": "Find documents in a MongoDB collection.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "collection": {"type": "string"},
                "filter": {"type": "object", "default": {}},
                "projection": {"type": "object", "default": {}},
                "sort": {"type": "object", "default": {}},
                "limit": {"type": "number", "default": 20},
            },
            "required": ["collection"],
        },
    },
    "mongodb_aggregate": {
        "name": "mongodb_aggregate",
        "description": "Run an aggregation pipeline in a MongoDB collection.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "collection": {"type": "string"},
                "pipeline": {"type": "array", "default": []},
                "limit": {"type": "number", "default": 50},
            },
            "required": ["collection", "pipeline"],
        },
    },
}


def _text_response(text: str) -> dict[str, Any]:
    return {"content": [{"type": "text", "text": text}]}


def _json_response(data: Any) -> dict[str, Any]:
    return _text_response(json.dumps(data, ensure_ascii=False, indent=2))


def _run_mongosh(script: str) -> Any:
    cmd = [
        "docker",
        "exec",
        "-i",
        CONTAINER,
        "mongosh",
        MONGO_URI,
        "--quiet",
        "--eval",
        script,
    ]
    proc = subprocess.run(
        cmd,
        text=True,
        capture_output=True,
        timeout=TIMEOUT_SECONDS,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.strip() or proc.stdout.strip())
    output = proc.stdout.strip()
    if not output:
        return None
    return json.loads(output)


def call_tool(name: str, args: dict[str, Any]) -> dict[str, Any]:
    if name == "mongodb_list_collections":
        return _json_response(_run_mongosh("JSON.stringify(db.getCollectionNames().sort())"))

    if name == "mongodb_count_documents":
        collection = json.dumps(args["collection"])
        filter_doc = json.dumps(args.get("filter") or {})
        script = f"JSON.stringify({{count: db.getCollection({collection}).countDocuments({filter_doc})}})"
        return _json_response(_run_mongosh(script))

    if name == "mongodb_find":
        collection = json.dumps(args["collection"])
        filter_doc = json.dumps(args.get("filter") or {})
        projection = json.dumps(args.get("projection") or {})
        sort = json.dumps(args.get("sort") or {})
        limit = max(0, min(int(args.get("limit", 20)), 200))
        script = (
            "JSON.stringify("
            f"db.getCollection({collection}).find({filter_doc}, {projection})"
            f".sort({sort}).limit({limit}).toArray()"
            ")"
        )
        return _json_response(_run_mongosh(script))

    if name == "mongodb_aggregate":
        collection = json.dumps(args["collection"])
        pipeline = list(args.get("pipeline") or [])
        limit = max(0, min(int(args.get("limit", 50)), 500))
        pipeline.append({"$limit": limit})
        script = f"JSON.stringify(db.getCollection({collection}).aggregate({json.dumps(pipeline)}).toArray())"
        return _json_response(_run_mongosh(script))

    raise ValueError(f"Unknown tool: {name}")


class MCPStdioServer:
    def _send(self, message: dict[str, Any]) -> None:
        sys.stdout.write(json.dumps(message, ensure_ascii=False) + "\n")
        sys.stdout.flush()

    def _recv(self) -> dict[str, Any] | None:
        line = sys.stdin.readline()
        if not line:
            return None
        return json.loads(line.lstrip("\ufeff"))

    def handle(self, message: dict[str, Any]) -> None:
        method = message.get("method")
        msg_id = message.get("id")
        try:
            if method == "initialize":
                self._send({
                    "jsonrpc": "2.0",
                    "id": msg_id,
                    "result": {
                        "protocolVersion": "2024-11-05",
                        "capabilities": {"tools": {}},
                        "serverInfo": {"name": "user-mongodb", "version": "1.0.0"},
                    },
                })
            elif method == "notifications/initialized":
                return
            elif method == "tools/list":
                self._send({"jsonrpc": "2.0", "id": msg_id, "result": {"tools": list(TOOLS.values())}})
            elif method == "tools/call":
                params = message.get("params", {})
                result = call_tool(params.get("name"), params.get("arguments") or {})
                self._send({"jsonrpc": "2.0", "id": msg_id, "result": result})
            elif method == "shutdown":
                self._send({"jsonrpc": "2.0", "id": msg_id, "result": None})
            else:
                self._send({
                    "jsonrpc": "2.0",
                    "id": msg_id,
                    "error": {"code": -32601, "message": f"Method not found: {method}"},
                })
        except Exception as exc:
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": _text_response(f"MongoDB error: {type(exc).__name__}: {exc}")})

    def run(self) -> None:
        while True:
            message = self._recv()
            if message is None:
                break
            self.handle(message)


if __name__ == "__main__":
    MCPStdioServer().run()
