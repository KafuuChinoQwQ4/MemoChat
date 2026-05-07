from __future__ import annotations

import asyncio
import json
import time
from typing import Awaitable, Callable
from urllib.parse import quote

import structlog

from config import AgentQueueConfig, RabbitMQQueueConfig, RedpandaQueueConfig
from harness.contracts import AgentTask

logger = structlog.get_logger()

TaskHandler = Callable[[str], Awaitable[None]]


def _now_ms() -> int:
    return int(time.time() * 1000)


def _task_message(task: AgentTask) -> dict:
    return {
        "event": "run",
        "task_id": task.task_id,
        "uid": int(task.payload.get("uid") or task.metadata.get("uid") or 0),
        "priority": task.priority,
        "created_at": _now_ms(),
    }


class RedpandaTaskEventPublisher:
    def __init__(self, config: RedpandaQueueConfig):
        self._config = config
        self._producer = None
        self._proxy_client = None
        self._started = False

    @property
    def started(self) -> bool:
        return self._started

    async def start(self) -> bool:
        if not self._config.enabled:
            return False
        try:
            from aiokafka import AIOKafkaProducer
        except Exception as exc:
            logger.warning("agent_queue.redpanda.import_failed", error=str(exc))
            return await self._start_proxy()

        try:
            brokers = [item.strip() for item in self._config.bootstrap_servers.split(",") if item.strip()]
            self._producer = AIOKafkaProducer(
                bootstrap_servers=brokers,
                client_id=self._config.client_id,
            )
            await self._producer.start()
            self._started = True
            logger.info(
                "agent_queue.redpanda.started",
                brokers=self._config.bootstrap_servers,
                topic=self._config.task_events_topic,
            )
            return True
        except Exception as exc:
            logger.warning("agent_queue.redpanda.start_failed", error=str(exc))
            self._producer = None
            self._started = False
            return await self._start_proxy()

    async def _start_proxy(self) -> bool:
        if not self._config.proxy_fallback_enabled or not self._config.proxy_url:
            return False
        try:
            import httpx

            self._proxy_client = httpx.AsyncClient(
                base_url=self._config.proxy_url.rstrip("/"),
                timeout=max(float(self._config.publish_timeout_sec), 0.5),
            )
            response = await self._proxy_client.get("/brokers")
            response.raise_for_status()
            self._started = True
            logger.info(
                "agent_queue.redpanda.proxy_started",
                proxy_url=self._config.proxy_url,
                topic=self._config.task_events_topic,
            )
            return True
        except Exception as exc:
            logger.warning("agent_queue.redpanda.proxy_start_failed", error=str(exc))
            if self._proxy_client is not None:
                with contextlib_suppress_log("agent_queue.redpanda.proxy_close_failed"):
                    await self._proxy_client.aclose()
            self._proxy_client = None
            self._started = False
            return False

    async def stop(self) -> None:
        producer = self._producer
        proxy_client = self._proxy_client
        self._producer = None
        self._proxy_client = None
        self._started = False
        if producer is not None:
            with contextlib_suppress_log("agent_queue.redpanda.stop_failed"):
                await producer.stop()
        if proxy_client is not None:
            with contextlib_suppress_log("agent_queue.redpanda.proxy_stop_failed"):
                await proxy_client.aclose()

    async def publish(self, event_type: str, task: AgentTask, extra: dict | None = None) -> bool:
        if not self._started or (self._producer is None and self._proxy_client is None):
            return False
        payload = {
            "event": event_type,
            "task_id": task.task_id,
            "status": task.status,
            "trace_id": task.trace_id,
            "uid": int(task.payload.get("uid") or task.metadata.get("uid") or 0),
            "title": task.title,
            "created_at": task.created_at,
            "updated_at": task.updated_at,
            "emitted_at": _now_ms(),
            "metadata": dict(task.metadata),
        }
        if extra:
            payload["extra"] = dict(extra)

        try:
            encoded = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
            if self._producer is None and self._proxy_client is not None:
                response = await self._proxy_client.post(
                    f"/topics/{self._config.task_events_topic}",
                    headers={
                        "Content-Type": "application/vnd.kafka.json.v2+json",
                        "Accept": "application/vnd.kafka.v2+json",
                    },
                    json={
                        "records": [
                            {
                                "key": task.task_id,
                                "value": payload,
                            }
                        ]
                    },
                )
                response.raise_for_status()
                return True

            await asyncio.wait_for(
                self._producer.send_and_wait(
                    self._config.task_events_topic,
                    key=task.task_id.encode("utf-8"),
                    value=encoded,
                ),
                timeout=max(float(self._config.publish_timeout_sec), 0.1),
            )
            return True
        except Exception as exc:
            if self._producer is not None and self._proxy_client is None:
                producer = self._producer
                self._producer = None
                with contextlib_suppress_log("agent_queue.redpanda.stop_failed"):
                    await producer.stop()
                if await self._start_proxy():
                    return await self.publish(event_type, task, extra=extra)
            logger.warning(
                "agent_queue.redpanda.publish_failed",
                task_id=task.task_id,
                event=event_type,
                error=str(exc),
            )
            return False


class RabbitMQTaskQueue:
    def __init__(self, config: RabbitMQQueueConfig):
        self._config = config
        self._connection = None
        self._channel = None
        self._exchange = None
        self._queue = None
        self._consumer_tag = ""
        self._handler: TaskHandler | None = None
        self._started = False

    @property
    def started(self) -> bool:
        return self._started

    async def start(self, handler: TaskHandler) -> bool:
        if not self._config.enabled:
            return False
        try:
            import aio_pika
        except Exception as exc:
            logger.warning("agent_queue.rabbitmq.import_failed", error=str(exc))
            return False

        self._handler = handler
        try:
            self._connection = await aio_pika.connect_robust(
                self._url(),
                client_properties={"connection_name": "memochat-ai-orchestrator"},
            )
            self._channel = await self._connection.channel()
            await self._channel.set_qos(prefetch_count=max(int(self._config.prefetch_count), 1))
            self._exchange = await self._channel.declare_exchange(
                self._config.exchange,
                aio_pika.ExchangeType.DIRECT,
                durable=True,
            )
            await self._channel.declare_exchange(
                self._config.dlx_exchange,
                aio_pika.ExchangeType.DIRECT,
                durable=True,
            )
            self._queue = await self._channel.declare_queue(
                self._config.task_queue,
                durable=True,
            )
            await self._queue.bind(self._exchange, routing_key=self._config.task_routing_key)
            self._consumer_tag = await self._queue.consume(self._consume)
            self._started = True
            logger.info(
                "agent_queue.rabbitmq.started",
                host=self._config.host,
                port=self._config.port,
                queue=self._config.task_queue,
                prefetch=self._config.prefetch_count,
            )
            return True
        except Exception as exc:
            logger.warning("agent_queue.rabbitmq.start_failed", error=str(exc))
            await self.stop()
            return False

    async def stop(self) -> None:
        self._started = False
        if self._queue is not None and self._consumer_tag:
            with contextlib_suppress_log("agent_queue.rabbitmq.cancel_failed"):
                await self._queue.cancel(self._consumer_tag)
        self._consumer_tag = ""
        if self._connection is not None:
            with contextlib_suppress_log("agent_queue.rabbitmq.close_failed"):
                await self._connection.close()
        self._connection = None
        self._channel = None
        self._exchange = None
        self._queue = None

    async def publish(self, task: AgentTask) -> None:
        if not self._started or self._exchange is None:
            raise RuntimeError("rabbitmq task queue is not started")
        import aio_pika

        payload = _task_message(task)
        message = aio_pika.Message(
            body=json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8"),
            content_type="application/json",
            delivery_mode=aio_pika.DeliveryMode.PERSISTENT,
            message_id=task.task_id,
            headers={"memochat_event": "agent_task_run"},
        )
        await asyncio.wait_for(
            self._exchange.publish(message, routing_key=self._config.task_routing_key),
            timeout=max(float(self._config.publish_timeout_sec), 0.1),
        )

    async def _consume(self, message) -> None:
        async with message.process(requeue=False):
            raw = message.body.decode("utf-8")
            data = json.loads(raw)
            task_id = str(data.get("task_id") or "").strip()
            if not task_id:
                raise ValueError("agent task message missing task_id")
            if self._handler is None:
                raise RuntimeError("agent task handler is not configured")
            await self._handler(task_id)

    def _url(self) -> str:
        username = quote(self._config.username, safe="")
        password = quote(self._config.password, safe="")
        if self._config.vhost in {"", "/"}:
            vhost = ""
        else:
            vhost = quote(self._config.vhost.strip("/"), safe="")
        return f"amqp://{username}:{password}@{self._config.host}:{self._config.port}/{vhost}"


class AgentTaskBus:
    def __init__(self, config: AgentQueueConfig):
        self._config = config
        self._rabbitmq = RabbitMQTaskQueue(config.rabbitmq)
        self._redpanda = RedpandaTaskEventPublisher(config.redpanda)
        self._queue_ready = False

    @property
    def queue_ready(self) -> bool:
        return self._queue_ready

    @property
    def backend_name(self) -> str:
        parts: list[str] = []
        if self._rabbitmq.started:
            parts.append("rabbitmq")
        if self._redpanda.started:
            parts.append("redpanda")
        return "+".join(parts) if parts else "local"

    async def start(self, handler: TaskHandler) -> bool:
        if not self._config.enabled:
            return False
        await self._redpanda.start()
        self._queue_ready = await self._rabbitmq.start(handler)
        return self._queue_ready

    async def stop(self) -> None:
        self._queue_ready = False
        await asyncio.gather(
            self._rabbitmq.stop(),
            self._redpanda.stop(),
            return_exceptions=True,
        )

    async def enqueue_task(self, task: AgentTask) -> None:
        await self._rabbitmq.publish(task)

    async def publish_event(self, event_type: str, task: AgentTask, extra: dict | None = None) -> None:
        await self._redpanda.publish(event_type, task, extra=extra)


class contextlib_suppress_log:
    def __init__(self, event_name: str):
        self._event_name = event_name

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        if exc is not None:
            logger.warning(self._event_name, error=str(exc))
        return True
