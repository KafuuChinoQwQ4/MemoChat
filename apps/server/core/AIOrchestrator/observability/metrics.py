from __future__ import annotations

from dataclasses import dataclass, field
from threading import Lock


def _escape_label_value(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


@dataclass
class _CounterFamily:
    name: str
    documentation: str
    label_names: tuple[str, ...] = ()
    _values: dict[tuple[str, ...], int] = field(default_factory=dict)
    _lock: Lock = field(default_factory=Lock)

    def inc(self, amount: int = 1, **labels: str) -> None:
        key = tuple(str(labels.get(label, "")) for label in self.label_names)
        with self._lock:
            self._values[key] = self._values.get(key, 0) + amount

    def render(self) -> str:
        lines = [
            f"# HELP {self.name} {self.documentation}",
            f"# TYPE {self.name} counter",
        ]
        with self._lock:
            items = list(self._values.items())
        if not items:
            lines.append(f"{self.name} 0")
            return "\n".join(lines)

        for key, value in sorted(items):
            if self.label_names:
                labels = ",".join(
                    f'{label}="{_escape_label_value(label_value)}"'
                    for label, label_value in zip(self.label_names, key)
                )
                lines.append(f"{self.name}{{{labels}}} {value}")
            else:
                lines.append(f"{self.name} {value}")
        return "\n".join(lines)


class AIMetricsRegistry:
    def __init__(self) -> None:
        self.http_requests = _CounterFamily(
            name="memochat_ai_http_requests_total",
            documentation="Total AIOrchestrator HTTP requests by route and status",
            label_names=("route", "status"),
        )
        self.ollama_retries = _CounterFamily(
            name="memochat_ai_ollama_retries_total",
            documentation="Total Ollama retry attempts by operation and reason",
            label_names=("operation", "reason"),
        )
        self.ollama_ready_probes = _CounterFamily(
            name="memochat_ai_ollama_ready_probes_total",
            documentation="Total Ollama readiness probes by status",
            label_names=("status",),
        )

    def render_text(self) -> str:
        return "\n".join(
            [
                self.http_requests.render(),
                self.ollama_retries.render(),
                self.ollama_ready_probes.render(),
                "",
            ]
        )


ai_metrics = AIMetricsRegistry()
