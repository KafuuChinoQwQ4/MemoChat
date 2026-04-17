"""
Langfuse 可观测性集成
"""
from config import settings


def init_langfuse():
    """初始化 Langfuse 追踪"""
    if not settings.observability.enabled or settings.observability.backend not in ("langfuse", "both"):
        return

    try:
        import langfuse
        langfuse_langfuse = langfuse.langfuse_context
    except ImportError:
        return
