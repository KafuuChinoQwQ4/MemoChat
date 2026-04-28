#!/usr/bin/env python3
import json
import subprocess
import sys
from typing import Any

CONTAINER = "memochat-redpanda"
BROKERS = "127.0.0.1:19092"
TIMEOUT_SECONDS = 30

TOOLS = {
    "redpanda_cluster_info": {
        "name": "redpanda_cluster_info",
        "description": "Show Redpanda cluster information.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "redpanda_list_topics": {
        "name": "redpanda_list_topics",
        "description": "List Redpanda topics.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "redpanda_describe_topic": {
        "name": "redpanda_describe_topic",
        "description": "Describe one Redpanda topic.",
        "inputSchema": {
            "type": "object",
            "properties": {"topic": {"type": "string"}},
            "required": ["topic"],
        },
    },
    "redpanda_topic_offsets": {
        "name": "redpanda_topic_offsets",
        "description": "Show offsets for one Redpanda topic.",
        "inputSchema": {
            "type": "object",
            "properties": {"topic": {"type": "string"}},
            "required": ["topic"],
        },
    },
}


def _text_response(text: str) -> dict[str, Any]:
    return {"content": [{"type": "text", "text": text}]}


def _run_rpk(args: list[str]) -> str:
    cmd = ["docker", "exec", CONTAINER, "rpk", *args, "--brokers", BROKERS]
    proc = subprocess.run(
        cmd,
        text=True,
        capture_output=True,
        timeout=TIMEOUT_SECONDS,
    )
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.strip() or proc.stdout.strip())
    return proc.stdout.strip()


def call_tool(name: str, args: dict[str, Any]) -> dict[str, Any]:
    if name == "redpanda_cluster_info":
        return _text_response(_run_rpk(["cluster", "info"]))
    if name == "redpanda_list_topics":
        return _text_response(_run_rpk(["topic", "list"]))
    if name == "redpanda_describe_topic":
        return _text_response(_run_rpk(["topic", "describe", args["topic"]]))
    if name == "redpanda_topic_offsets":
        return _text_response(_run_rpk(["topic", "describe", args["topic"], "--print-partitions"]))
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
                        "serverInfo": {"name": "user-redpanda", "version": "1.0.0"},
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
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": _text_response(f"Redpanda error: {type(exc).__name__}: {exc}")})

    def run(self) -> None:
        while True:
            message = self._recv()
            if message is None:
                break
            self.handle(message)


if __name__ == "__main__":
    MCPStdioServer().run()
