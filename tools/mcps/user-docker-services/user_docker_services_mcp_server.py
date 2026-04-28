#!/usr/bin/env python3
import base64
import json
import sys
import urllib.error
import urllib.parse
import urllib.request
from typing import Any

try:
    from minio import Minio
except Exception:
    Minio = None

HTTP_TIMEOUT = 10

ENDPOINTS = {
    "prometheus": "http://127.0.0.1:9090",
    "grafana": "http://127.0.0.1:3000",
    "loki": "http://127.0.0.1:3100",
    "tempo": "http://127.0.0.1:3200",
    "influxdb": "http://127.0.0.1:8086",
    "rabbitmq": "http://127.0.0.1:15672",
    "cadvisor": "http://127.0.0.1:8088",
}

AUTH = {
    "grafana": ("admin", "admin"),
    "rabbitmq": ("memochat", "123456"),
}

INFLUX_TOKEN = "my-super-secret-admin-token"
INFLUX_ORG = "memochat"
INFLUX_BUCKET = "metrics"

TOOLS = {
    "minio_list_buckets": {
        "name": "minio_list_buckets",
        "description": "List MinIO buckets.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "minio_list_objects": {
        "name": "minio_list_objects",
        "description": "List objects in a MinIO bucket.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "bucket": {"type": "string"},
                "prefix": {"type": "string", "default": ""},
                "recursive": {"type": "boolean", "default": False},
                "limit": {"type": "number", "default": 100},
            },
            "required": ["bucket"],
        },
    },
    "rabbitmq_overview": {
        "name": "rabbitmq_overview",
        "description": "Get RabbitMQ management overview.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "rabbitmq_list_queues": {
        "name": "rabbitmq_list_queues",
        "description": "List RabbitMQ queues.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "prometheus_query": {
        "name": "prometheus_query",
        "description": "Run an instant Prometheus query.",
        "inputSchema": {
            "type": "object",
            "properties": {"query": {"type": "string"}, "time": {"type": "string"}},
            "required": ["query"],
        },
    },
    "prometheus_targets": {
        "name": "prometheus_targets",
        "description": "List Prometheus scrape targets.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "grafana_datasources": {
        "name": "grafana_datasources",
        "description": "List Grafana datasources.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "grafana_search_dashboards": {
        "name": "grafana_search_dashboards",
        "description": "Search Grafana dashboards.",
        "inputSchema": {
            "type": "object",
            "properties": {"query": {"type": "string", "default": ""}},
        },
    },
    "loki_query": {
        "name": "loki_query",
        "description": "Run an instant Loki LogQL query.",
        "inputSchema": {
            "type": "object",
            "properties": {"query": {"type": "string"}, "limit": {"type": "number", "default": 100}},
            "required": ["query"],
        },
    },
    "tempo_services": {
        "name": "tempo_services",
        "description": "List Tempo-discovered service names.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    "tempo_search_traces": {
        "name": "tempo_search_traces",
        "description": "Search Tempo traces by tags/query.",
        "inputSchema": {
            "type": "object",
            "properties": {"tags": {"type": "string", "default": ""}, "limit": {"type": "number", "default": 20}},
        },
    },
    "influx_query": {
        "name": "influx_query",
        "description": "Run a Flux query against InfluxDB.",
        "inputSchema": {
            "type": "object",
            "properties": {"query": {"type": "string"}, "org": {"type": "string", "default": INFLUX_ORG}},
            "required": ["query"],
        },
    },
    "cadvisor_metrics": {
        "name": "cadvisor_metrics",
        "description": "Fetch cAdvisor Prometheus metrics text, optionally filtered by substring.",
        "inputSchema": {
            "type": "object",
            "properties": {"contains": {"type": "string", "default": ""}, "limit": {"type": "number", "default": 200}},
        },
    },
}


def _json_response(data: Any) -> dict[str, Any]:
    return {"content": [{"type": "text", "text": json.dumps(data, ensure_ascii=False, indent=2)}]}


def _text_response(text: str) -> dict[str, Any]:
    return {"content": [{"type": "text", "text": text}]}


def _http_json(service: str, path: str, query: dict[str, Any] | None = None, method: str = "GET", body: Any = None, headers: dict[str, str] | None = None) -> Any:
    url = ENDPOINTS[service] + path
    if query:
        clean = {k: str(v) for k, v in query.items() if v is not None and v != ""}
        if clean:
            url += "?" + urllib.parse.urlencode(clean)
    request_headers = dict(headers or {})
    data = None
    if body is not None:
        data = json.dumps(body).encode("utf-8") if not isinstance(body, (bytes, bytearray)) else body
        request_headers.setdefault("Content-Type", "application/json")
    if service in AUTH:
        user, password = AUTH[service]
        token = base64.b64encode(f"{user}:{password}".encode()).decode()
        request_headers["Authorization"] = f"Basic {token}"
    req = urllib.request.Request(url, data=data, headers=request_headers, method=method)
    with urllib.request.urlopen(req, timeout=HTTP_TIMEOUT) as resp:
        raw = resp.read()
        content_type = resp.headers.get("content-type", "")
        if "json" in content_type:
            return json.loads(raw.decode("utf-8"))
        return raw.decode("utf-8", errors="replace")


def _minio_client():
    if Minio is None:
        raise RuntimeError("Python package 'minio' is not installed")
    return Minio("127.0.0.1:9000", access_key="memochat_admin", secret_key="MinioPass2026!", secure=False)


def call_tool(name: str, args: dict[str, Any]) -> dict[str, Any]:
    if name == "minio_list_buckets":
        buckets = _minio_client().list_buckets()
        return _json_response([{"name": b.name, "creation_date": b.creation_date.isoformat() if b.creation_date else None} for b in buckets])
    if name == "minio_list_objects":
        limit = int(args.get("limit", 100))
        objects = []
        for obj in _minio_client().list_objects(args["bucket"], prefix=args.get("prefix", ""), recursive=bool(args.get("recursive", False))):
            objects.append({"object_name": obj.object_name, "size": obj.size, "last_modified": obj.last_modified.isoformat() if obj.last_modified else None})
            if len(objects) >= limit:
                break
        return _json_response(objects)
    if name == "rabbitmq_overview":
        return _json_response(_http_json("rabbitmq", "/api/overview"))
    if name == "rabbitmq_list_queues":
        queues = _http_json("rabbitmq", "/api/queues")
        return _json_response([{"name": q.get("name"), "vhost": q.get("vhost"), "messages": q.get("messages"), "consumers": q.get("consumers")} for q in queues])
    if name == "prometheus_query":
        return _json_response(_http_json("prometheus", "/api/v1/query", {"query": args["query"], "time": args.get("time")}))
    if name == "prometheus_targets":
        return _json_response(_http_json("prometheus", "/api/v1/targets"))
    if name == "grafana_datasources":
        return _json_response(_http_json("grafana", "/api/datasources"))
    if name == "grafana_search_dashboards":
        return _json_response(_http_json("grafana", "/api/search", {"query": args.get("query", "")}))
    if name == "loki_query":
        return _json_response(_http_json("loki", "/loki/api/v1/query", {"query": args["query"], "limit": int(args.get("limit", 100))}))
    if name == "tempo_services":
        return _json_response(_http_json("tempo", "/api/search/tags/service.name/values"))
    if name == "tempo_search_traces":
        return _json_response(_http_json("tempo", "/api/search", {"tags": args.get("tags", ""), "limit": int(args.get("limit", 20))}))
    if name == "influx_query":
        headers = {"Authorization": f"Token {INFLUX_TOKEN}", "Accept": "application/csv", "Content-Type": "application/vnd.flux"}
        org = args.get("org", INFLUX_ORG)
        return _text_response(_http_json("influxdb", "/api/v2/query", {"org": org}, method="POST", body=args["query"].encode("utf-8"), headers=headers))
    if name == "cadvisor_metrics":
        text = _http_json("cadvisor", "/metrics")
        contains = args.get("contains", "")
        limit = int(args.get("limit", 200))
        lines = [line for line in text.splitlines() if not contains or contains in line]
        return _text_response("\n".join(lines[:limit]))
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
                self._send({"jsonrpc": "2.0", "id": msg_id, "result": {"protocolVersion": "2024-11-05", "capabilities": {"tools": {}}, "serverInfo": {"name": "user-docker-services", "version": "1.0.0"}}})
            elif method == "notifications/initialized":
                return
            elif method == "tools/list":
                self._send({"jsonrpc": "2.0", "id": msg_id, "result": {"tools": list(TOOLS.values())}})
            elif method == "tools/call":
                params = message.get("params", {})
                self._send({"jsonrpc": "2.0", "id": msg_id, "result": call_tool(params.get("name"), params.get("arguments") or {})})
            elif method == "shutdown":
                self._send({"jsonrpc": "2.0", "id": msg_id, "result": None})
            else:
                self._send({"jsonrpc": "2.0", "id": msg_id, "error": {"code": -32601, "message": f"Method not found: {method}"}})
        except Exception as exc:
            self._send({"jsonrpc": "2.0", "id": msg_id, "result": _text_response(f"Error: {type(exc).__name__}: {exc}")})

    def run(self) -> None:
        while True:
            message = self._recv()
            if message is None:
                break
            self.handle(message)


if __name__ == "__main__":
    MCPStdioServer().run()
