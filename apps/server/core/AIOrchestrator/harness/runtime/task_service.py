from __future__ import annotations

import asyncio
import contextlib
import json
import time
import uuid
from pathlib import Path
from types import SimpleNamespace
from typing import Any

from harness.contracts import AgentTask


def _now_ms() -> int:
    return int(time.time() * 1000)


class AgentTaskService:
    def __init__(
        self,
        agent_service,
        storage_dir: str | Path | None = None,
        auto_run: bool = True,
    ):
        self._agent_service = agent_service
        root_dir = Path(__file__).resolve().parents[2]
        self._storage_dir = Path(storage_dir) if storage_dir is not None else root_dir / ".runtime" / "agent_tasks"
        self._auto_run = auto_run
        self._tasks: dict[str, AgentTask] = {}
        self._background: dict[str, asyncio.Task] = {}
        self._lock = asyncio.Lock()
        self._shutting_down = False

    async def startup(self) -> None:
        self._shutting_down = False
        self._storage_dir.mkdir(parents=True, exist_ok=True)
        for path in self._storage_dir.glob("*.json"):
            with contextlib.suppress(Exception):
                task = self._task_from_dict(json.loads(path.read_text(encoding="utf-8")))
                if task.status in {"queued", "running"}:
                    task.status = "paused"
                    task.updated_at = _now_ms()
                    self._append_checkpoint(task, "paused", "Task paused after service restart.")
                    await self._persist(task)
                self._tasks[task.task_id] = task

    async def shutdown(self) -> None:
        self._shutting_down = True
        pending = list(self._background.values())
        for handle in pending:
            handle.cancel()
        if pending:
            await asyncio.gather(*pending, return_exceptions=True)
        self._background.clear()

    async def create_task(
        self,
        uid: int,
        title: str,
        content: str,
        session_id: str = "",
        model_type: str = "",
        model_name: str = "",
        skill_name: str = "",
        metadata: dict[str, Any] | None = None,
    ) -> AgentTask:
        task_id = uuid.uuid4().hex
        now = _now_ms()
        task = AgentTask(
            task_id=task_id,
            title=title.strip() or content.strip()[:48] or "Agent task",
            status="queued",
            description=content.strip()[:240],
            payload={
                "uid": uid,
                "content": content,
                "session_id": session_id,
                "model_type": model_type,
                "model_name": model_name,
                "skill_name": skill_name,
                "metadata": dict(metadata or {}),
            },
            created_at=now,
            updated_at=now,
            metadata={"uid": uid},
        )
        self._append_checkpoint(task, "queued", "Task accepted.")
        async with self._lock:
            self._tasks[task.task_id] = task
            await self._persist(task)
            self._schedule(task)
        return task

    async def list_tasks(self, uid: int, limit: int = 50) -> list[AgentTask]:
        await self._ensure_loaded()
        limit = min(max(int(limit or 50), 1), 200)
        tasks = [
            task
            for task in self._tasks.values()
            if int(task.payload.get("uid") or task.metadata.get("uid") or 0) == uid
        ]
        tasks.sort(key=lambda item: item.updated_at or item.created_at, reverse=True)
        return tasks[:limit]

    async def get_task(self, task_id: str) -> AgentTask | None:
        await self._ensure_loaded()
        return self._tasks.get(task_id)

    async def cancel_task(self, task_id: str) -> AgentTask | None:
        task = await self.get_task(task_id)
        if task is None:
            return None
        handle = self._background.pop(task_id, None)
        if handle is not None and not handle.done():
            handle.cancel()
        task.status = "canceled"
        task.cancelled_at = _now_ms()
        task.updated_at = task.cancelled_at
        self._append_checkpoint(task, "canceled", "Task canceled by user.")
        await self._persist(task)
        return task

    async def resume_task(self, task_id: str) -> AgentTask | None:
        task = await self.get_task(task_id)
        if task is None:
            return None
        if task.status in {"queued", "running"}:
            return task
        task.status = "queued"
        task.error = ""
        task.cancelled_at = 0
        task.completed_at = 0
        task.updated_at = _now_ms()
        self._append_checkpoint(task, "queued", "Task queued for resume.")
        await self._persist(task)
        self._schedule(task)
        return task

    async def _ensure_loaded(self) -> None:
        if self._tasks:
            return
        await self.startup()

    def _schedule(self, task: AgentTask) -> None:
        if not self._auto_run:
            return
        existing = self._background.get(task.task_id)
        if existing is not None and not existing.done():
            return
        self._background[task.task_id] = asyncio.create_task(self._run_task(task.task_id))

    async def _run_task(self, task_id: str) -> None:
        task = self._tasks.get(task_id)
        if task is None:
            return
        try:
            task.status = "running"
            task.updated_at = _now_ms()
            self._append_checkpoint(task, "running", "Agent run started.")
            await self._persist(task)

            payload = dict(task.payload)
            metadata = dict(payload.get("metadata") or {})
            if payload.get("skill_name"):
                metadata.setdefault("skill_name", payload.get("skill_name"))
            request = SimpleNamespace(
                uid=int(payload.get("uid") or 0),
                session_id=payload.get("session_id") or "",
                content=payload.get("content") or "",
                model_type=payload.get("model_type") or "",
                model_name=payload.get("model_name") or "",
                deployment_preference=payload.get("deployment_preference") or "any",
                skill_name=payload.get("skill_name") or "",
                feature_type="",
                target_lang=payload.get("target_lang") or "",
                requested_tools=payload.get("requested_tools") or [],
                tool_arguments=payload.get("tool_arguments") or {},
                metadata=metadata,
            )
            result = await self._agent_service.run_turn(request)

            task.status = "completed"
            task.trace_id = result.trace_id
            task.result = {
                "content": result.content,
                "tokens": result.tokens,
                "model": result.model,
                "skill": result.skill,
                "feedback_summary": result.feedback_summary,
                "observations": result.observations,
            }
            task.completed_at = _now_ms()
            task.updated_at = task.completed_at
            self._append_checkpoint(task, "completed", "Agent run completed.")
            await self._persist(task)
        except asyncio.CancelledError:
            task.updated_at = _now_ms()
            if self._shutting_down:
                task.status = "paused"
                self._append_checkpoint(task, "paused", "Task paused during service shutdown.")
            else:
                task.status = "canceled"
                task.cancelled_at = task.updated_at
                self._append_checkpoint(task, "canceled", "Task execution was canceled.")
            await self._persist(task)
            raise
        except Exception as exc:
            task.status = "failed"
            task.error = f"{type(exc).__name__}: {str(exc)[:300]}"
            task.updated_at = _now_ms()
            self._append_checkpoint(task, "failed", task.error)
            await self._persist(task)
        finally:
            self._background.pop(task_id, None)

    def _task_from_dict(self, data: dict[str, Any]) -> AgentTask:
        allowed = set(AgentTask.__dataclass_fields__.keys())
        return AgentTask(**{key: value for key, value in data.items() if key in allowed})

    def _append_checkpoint(self, task: AgentTask, status: str, message: str) -> None:
        task.checkpoints.append({"status": status, "message": message, "created_at": _now_ms()})

    async def _persist(self, task: AgentTask) -> None:
        self._storage_dir.mkdir(parents=True, exist_ok=True)
        path = self._storage_dir / f"{task.task_id}.json"
        tmp_path = path.with_suffix(".json.tmp")
        tmp_path.write_text(
            json.dumps(task.to_dict(), ensure_ascii=False, indent=2, sort_keys=True),
            encoding="utf-8",
        )
        tmp_path.replace(path)
