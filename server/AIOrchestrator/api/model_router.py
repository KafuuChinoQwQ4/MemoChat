"""
模型列表 API 路由
GET /models — 返回可用模型列表
"""
from fastapi import APIRouter

from schemas.api import ListModelsRsp, ModelInfo
from config import settings

router = APIRouter()


@router.get("", response_model=ListModelsRsp)
async def list_models():
    """
    返回所有可用模型列表。
    """
    models = []
    default_found = False
    default_model = settings.llm.default_model

    for backend, cfg, model_list_cls in [
        ("ollama", settings.llm.ollama, _make_ollama_models),
        ("openai", settings.llm.openai, _make_openai_models),
        ("claude", settings.llm.anthropic, _make_claude_models),
        ("kimi", settings.llm.kimi, _make_kimi_models),
    ]:
        if not cfg.enabled:
            continue

        for m in model_list_cls(cfg):
            is_default = m.model_name == default_model and not default_found
            if is_default:
                default_found = True
            models.append(ModelInfo(
                model_type=backend,
                model_name=m.name,
                display_name=m.display,
                is_enabled=cfg.enabled,
                context_window=m.context_window,
            ))

    return ListModelsRsp(
        code=0,
        models=models,
        default_model=models[0] if models else None,
    )


def _make_ollama_models(cfg):
    return cfg.models or [{"name": "qwen2.5:7b", "display": "Qwen 2.5 7B", "context_window": 8192}]


def _make_openai_models(cfg):
    return cfg.models or [{"name": "gpt-4o", "display": "GPT-4o", "context_window": 128000}]


def _make_claude_models(cfg):
    return cfg.models or [{"name": "claude-3-5-sonnet-20241022", "display": "Claude 3.5 Sonnet", "context_window": 200000}]


def _make_kimi_models(cfg):
    return cfg.models or [{"name": "moonshot-v1-8k", "display": "Kimi 8K", "context_window": 8192}]
