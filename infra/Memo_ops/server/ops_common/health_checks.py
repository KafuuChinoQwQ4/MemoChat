from __future__ import annotations

from urllib.error import URLError
from urllib.request import urlopen


def data_source_health(config: dict) -> dict[str, dict]:
    result = {}
    for name in ("grafana", "loki", "tempo"):
        url = (config.get("observability", {}) or {}).get(name, "")
        healthy = False
        if url:
            try:
                with urlopen(url, timeout=2.0) as response:
                    healthy = 200 <= int(response.status) < 500
            except (URLError, TimeoutError, ValueError):
                healthy = False
        result[name] = {"url": url, "healthy": healthy}
    return result
