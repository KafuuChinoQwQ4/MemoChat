from __future__ import annotations

import argparse

import uvicorn
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from Memo_ops.server.ops_server.routes import (
    create_admin_router,
    create_loadtests_router,
    create_logs_router,
    create_metrics_router,
    create_overview_router,
    create_system_router,
)
from Memo_ops.server.ops_server.runtime import DEFAULT_CONFIG, OpsServerRuntime


def create_app(runtime: OpsServerRuntime | None = None) -> FastAPI:
    server_runtime = runtime or OpsServerRuntime()
    app = FastAPI(title="Memo_ops OpsServer", version="1.0.0")
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    @app.on_event("startup")
    def on_startup() -> None:
        try:
            server_runtime.ensure_schema()
        except Exception:
            pass

    @app.get("/healthz")
    def healthz() -> dict:
        return {"status": "ok", "service": "MemoOpsServer"}

    @app.get("/readyz")
    def readyz() -> dict:
        return {"status": "ready", "service": "MemoOpsServer"}

    app.include_router(create_overview_router(server_runtime))
    app.include_router(create_loadtests_router(server_runtime))
    app.include_router(create_logs_router(server_runtime))
    app.include_router(create_metrics_router(server_runtime))
    app.include_router(create_admin_router(server_runtime))
    app.include_router(create_system_router(server_runtime))
    return app


runtime = OpsServerRuntime()
app = create_app(runtime)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", default=str(DEFAULT_CONFIG))
    args = parser.parse_args()

    runtime.reload_config(args.config)
    uvicorn.run(
        "Memo_ops.server.ops_server.main:app",
        host=runtime.config["server"]["host"],
        port=int(runtime.config["server"]["port"]),
        reload=False,
    )


if __name__ == "__main__":
    main()
