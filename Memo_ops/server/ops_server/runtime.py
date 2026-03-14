from __future__ import annotations

from pathlib import Path
from typing import Callable

from Memo_ops.server.ops_common.config import load_yaml_config
from Memo_ops.server.ops_common.schema import init_schema
from Memo_ops.server.ops_common.storage import postgres_conn


DEFAULT_CONFIG = Path(__file__).resolve().parents[2] / "config" / "opsserver.yaml"


class OpsServerRuntime:
    def __init__(self, config_path: str | None = None) -> None:
        self.config_path = config_path or str(DEFAULT_CONFIG)
        self.config = load_yaml_config(self.config_path)
        self.startup_state = {"schema_ready": False, "startup_error": ""}

    def reload_config(self, config_path: str | None = None) -> dict:
        self.config_path = config_path or self.config_path
        self.config = load_yaml_config(self.config_path)
        self.startup_state = {"schema_ready": False, "startup_error": ""}
        return self.config

    def ensure_schema(self) -> None:
        if self.startup_state["schema_ready"]:
            return
        try:
            init_schema(self.config["postgresql"])
            self.startup_state["schema_ready"] = True
            self.startup_state["startup_error"] = ""
        except Exception as exc:
            self.startup_state["startup_error"] = str(exc)
            raise

    def guarded(self, action: Callable[[], dict]) -> dict:
        try:
            self.ensure_schema()
            return action()
        except Exception as exc:
            return {"error": str(exc), "startup_error": self.startup_state["startup_error"]}

    def with_conn(self, action: Callable) -> dict:
        def wrapped():
            with postgres_conn(self.config["postgresql"]) as conn:
                return action(conn)

        return self.guarded(wrapped)
