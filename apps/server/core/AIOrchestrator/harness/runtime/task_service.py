from __future__ import annotations

import asyncio
import contextlib
import json
import time
import uuid
from pathlib import Path
from types import SimpleNamespace
from typing import Any

import structlog
from config import settings
from harness.contracts import AgentTask
from harness.runtime.message_bus import AgentTaskBus
from observability.metrics import ai_metrics

logger = structlog.get_logger()


def _now_ms() -> int:
    return int(time.time() * 1000)


class AgentTaskService:
    def __init__(
        self,
        agent_service,
        storage_dir: str | Path | None = None,
        auto_run: bool = True,
        task_bus=None,
        worker_concurrency: int | None = None,
    ):
        self._agent_service = agent_service
        root_dir = Path(__file__).resolve().parents[2]
        self._storage_dir = Path(storage_dir) if storage_dir is not None else root_dir / ".runtime" / "agent_tasks"
        self._auto_run = auto_run
        self._tasks: dict[str, AgentTask] = {}
        self._background: dict[str, asyncio.Task] = {}
        self._dispatch_background: dict[str, asyncio.Task] = {}
        self._lock = asyncio.Lock()
        self._shutting_down = False
        self._queue_cfg = settings.agent_queue
        self._fallback_to_local = bool(self._queue_cfg.fallback_to_local)
        concurrency = worker_concurrency if worker_concurrency is not None else self._queue_cfg.worker_concurrency
        self._run_semaphore = asyncio.Semaphore(max(int(concurrency or 1), 1))
        self._task_bus = task_bus if task_bus is not None else AgentTaskBus(self._queue_cfg)
        self._queue_ready = False
        self._loaded_once = False

    async def startup(self) -> None:
        if self._loaded_once:
            return
        self._shutting_down = False
        self._storage_dir.mkdir(parents=True, exist_ok=True)
        loaded_tasks: list[AgentTask] = []
        for path in self._storage_dir.glob("*.json"):
            with contextlib.suppress(Exception):
                task = await self._read_task_file(path)
                if task.status in {"queued", "running"}:
                    if self._queue_cfg.resume_stale_on_start:
                        task.status = "queued"
                        task.updated_at = _now_ms()
                        self._append_checkpoint(task, "queued", "Task requeued after service restart.")
                    else:
                        task.status = "paused"
                        task.updated_at = _now_ms()
                        self._append_checkpoint(task, "paused", "Task paused after service restart.")
                    await self._persist(task)
                self._tasks[task.task_id] = task
                loaded_tasks.append(task)

        if self._auto_run and self._queue_cfg.enabled:
            try:
                self._queue_ready = await self._task_bus.start(self._consume_task)
            except Exception as exc:
                logger.warning("agent_task.queue_start_failed", error=str(exc))
                self._queue_ready = False

        for task in loaded_tasks:
            if task.status == "queued":
                self._schedule(task)
        self._loaded_once = True

    async def shutdown(self) -> None:
        self._shutting_down = True
        dispatch_handles = list(self._dispatch_background.values())
        run_handles = list(self._background.values())
        for handle in [*dispatch_handles, *run_handles]:
            handle.cancel()
        if dispatch_handles or run_handles:
            await asyncio.gather(*dispatch_handles, *run_handles, return_exceptions=True)
        self._dispatch_background.clear()
        self._background.clear()
        with contextlib.suppress(Exception):
            await self._task_bus.stop()
        self._queue_ready = False

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
        task_metadata = {"uid": uid, "queue_backend": self._backend_name()}
        task_metadata.update(dict(metadata or {}).get("task_metadata", {}))
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
            metadata=task_metadata,
        )
        self._append_checkpoint(task, "queued", "Task accepted.")
        async with self._lock:
            self._tasks[task.task_id] = task
            await self._persist(task)
            await self._emit_event("queued", task)
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
        task = self._tasks.get(task_id)
        if task is not None:
            return task
        return await self._load_task_from_disk(task_id)

    async def cancel_task(self, task_id: str) -> AgentTask | None:
        task = await self.get_task(task_id)
        if task is None:
            return None
        handle = self._background.pop(task_id, None)
        if handle is not None and not handle.done():
            handle.cancel()
        dispatch = self._dispatch_background.pop(task_id, None)
        if dispatch is not None and not dispatch.done():
            dispatch.cancel()
        changed = False
        async with self._lock:
            if task.status == "completed":
                return task
            if task.status != "canceled":
                task.status = "canceled"
                task.cancelled_at = _now_ms()
                task.updated_at = task.cancelled_at
                self._append_checkpoint(task, "canceled", "Task canceled by user.")
                changed = True
        if changed:
            await self._persist(task)
            await self._emit_event("canceled", task)
        return task

    async def resume_task(self, task_id: str) -> AgentTask | None:
        task = await self.get_task(task_id)
        if task is None:
            return None
        changed = False
        async with self._lock:
            if task.status in {"queued", "running"}:
                return task
            task.status = "queued"
            task.error = ""
            task.cancelled_at = 0
            task.completed_at = 0
            task.updated_at = _now_ms()
            self._append_checkpoint(task, "queued", "Task queued for resume.")
            changed = True
        if not changed:
            return task
        await self._persist(task)
        await self._emit_event("queued", task)
        self._schedule(task)
        return task

    async def _ensure_loaded(self) -> None:
        if self._loaded_once:
            return
        await self.startup()

    def _schedule(self, task: AgentTask) -> None:
        if not self._auto_run or self._shutting_down:
            return
        if self._queue_ready:
            existing = self._dispatch_background.get(task.task_id)
            if existing is not None and not existing.done():
                return
            self._dispatch_background[task.task_id] = asyncio.create_task(self._dispatch_to_queue(task.task_id))
            return
        self._schedule_local(task)

    def _schedule_local(self, task: AgentTask) -> None:
        existing = self._background.get(task.task_id)
        if existing is not None and not existing.done():
            return
        self._background[task.task_id] = asyncio.create_task(self._run_task(task.task_id))

    async def _dispatch_to_queue(self, task_id: str) -> None:
        try:
            task = await self.get_task(task_id)
            if task is None or task.status != "queued":
                return
            await self._task_bus.enqueue_task(task)
            task.metadata["queue_backend"] = self._backend_name()
            await self._persist(task)
            ai_metrics.agent_tasks.inc(backend=self._backend_name(), status="enqueued")
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            logger.warning("agent_task.queue_dispatch_failed", task_id=task_id, error=str(exc))
            task = await self.get_task(task_id)
            if task is not None and self._fallback_to_local:
                self._append_checkpoint(task, "queued", f"Queue dispatch failed; local fallback: {type(exc).__name__}.")
                await self._persist(task)
                self._schedule_local(task)
        finally:
            self._dispatch_background.pop(task_id, None)

    async def _consume_task(self, task_id: str) -> None:
        task = await self.get_task(task_id)
        if task is None:
            logger.warning("agent_task.queue_missing_task", task_id=task_id)
            return
        if task.status in {"completed", "canceled"}:
            logger.info("agent_task.queue_skip_terminal", task_id=task_id, status=task.status)
            return
        await self._run_task(task_id)

    async def _run_task(self, task_id: str) -> None:
        async with self._run_semaphore:
            await self._run_task_unlocked(task_id)

    async def _run_task_unlocked(self, task_id: str) -> None:
        task = await self._claim_queued_task(task_id)
        if task is None:
            return
        try:
            await self._persist(task)
            await self._emit_event("running", task)

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

            if await self._complete_running_task(task, result):
                await self._persist(task)
                await self._emit_event("completed", task, {"trace_id": task.trace_id})
        except asyncio.CancelledError:
            if await self._interrupt_running_task(task):
                await self._persist(task)
                await self._emit_event(task.status, task)
            raise
        except Exception as exc:
            if await self._fail_running_task(task, exc):
                await self._persist(task)
                await self._emit_event("failed", task, {"error": task.error})
        finally:
            self._background.pop(task_id, None)

    async def _claim_queued_task(self, task_id: str) -> AgentTask | None:
        task = await self.get_task(task_id)
        if task is None:
            return None
        async with self._lock:
            if task.status in {"completed", "canceled"}:
                return None
            if task.status != "queued":
                logger.info("agent_task.skip_non_runnable", task_id=task_id, status=task.status)
                return None
            task.status = "running"
            task.updated_at = _now_ms()
            self._append_checkpoint(task, "running", "Agent run started.")
            return task

    async def _complete_running_task(self, task: AgentTask, result) -> bool:
        async with self._lock:
            if task.status != "running":
                return False
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
            return True

    async def _interrupt_running_task(self, task: AgentTask) -> bool:
        async with self._lock:
            if task.status != "running":
                return False
            task.updated_at = _now_ms()
            if self._shutting_down:
                task.status = "paused"
                self._append_checkpoint(task, "paused", "Task paused during service shutdown.")
            else:
                task.status = "canceled"
                task.cancelled_at = task.updated_at
                self._append_checkpoint(task, "canceled", "Task execution was canceled.")
            return True

    async def _fail_running_task(self, task: AgentTask, exc: Exception) -> bool:
        async with self._lock:
            if task.status != "running":
                return False
            task.status = "failed"
            task.error = f"{type(exc).__name__}: {str(exc)[:300]}"
            task.updated_at = _now_ms()
            self._append_checkpoint(task, "failed", task.error)
            return True

    async def _load_task_from_disk(self, task_id: str) -> AgentTask | None:
        path = self._storage_dir / f"{task_id}.json"
        if not path.exists():
            return None
        try:
            task = await self._read_task_file(path)
        except Exception as exc:
            logger.warning("agent_task.load_failed", task_id=task_id, error=str(exc))
            return None
        self._tasks[task.task_id] = task
        return task

    async def _read_task_file(self, path: Path) -> AgentTask:
        text = await asyncio.to_thread(path.read_text, encoding="utf-8")
        return self._task_from_dict(json.loads(text))

    def _task_from_dict(self, data: dict[str, Any]) -> AgentTask:
        allowed = set(AgentTask.__dataclass_fields__.keys())
        return AgentTask(**{key: value for key, value in data.items() if key in allowed})

    def _append_checkpoint(self, task: AgentTask, status: str, message: str) -> None:
        task.checkpoints.append({"status": status, "message": message, "created_at": _now_ms()})

    async def _persist(self, task: AgentTask) -> None:
        payload = json.dumps(task.to_dict(), ensure_ascii=False, indent=2, sort_keys=True)

        def write_file() -> None:
            self._storage_dir.mkdir(parents=True, exist_ok=True)
            path = self._storage_dir / f"{task.task_id}.json"
            tmp_path = path.with_suffix(".json.tmp")
            tmp_path.write_text(payload, encoding="utf-8")
            tmp_path.replace(path)

        await asyncio.to_thread(write_file)

    async def _emit_event(self, event_type: str, task: AgentTask, extra: dict | None = None) -> None:
        ai_metrics.agent_tasks.inc(backend=self._backend_name(), status=event_type)
        if not self._queue_cfg.enabled:
            return
        with contextlib.suppress(Exception):
            await self._task_bus.publish_event(event_type, task, extra=extra)

    def _backend_name(self) -> str:
        backend_name = getattr(self._task_bus, "backend_name", "")
        return str(backend_name or "local")
