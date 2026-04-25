"""
模型列表 API 路由
"""
from fastapi import APIRouter

from config import settings
from harness import HarnessContainer
from schemas.api import ListModelsRsp, ModelInfo, ProviderInfo

router = APIRouter()


@router.get("", response_model=ListModelsRsp)
async def list_models():
    container = HarnessContainer.get_instance()
    endpoints = container.llm_registry.list_endpoints()
    model_items: list[ModelInfo] = []
    provider_items: list[ProviderInfo] = []
    default_model: ModelInfo | None = None

    for endpoint in endpoints:
        provider_items.append(
            ProviderInfo(
                provider_id=endpoint.provider_id,
                adapter=endpoint.adapter,
                deployment=endpoint.deployment,
                base_url=endpoint.base_url,
                default_model=endpoint.default_model,
                enabled=endpoint.enabled,
            )
        )
        for model in endpoint.models:
            model_info = ModelInfo(
                model_type=endpoint.provider_id,
                model_name=model.get("name", ""),
                display_name=model.get("display", model.get("name", "")),
                is_enabled=endpoint.enabled,
                context_window=model.get("context_window", 0),
                provider_id=endpoint.provider_id,
                adapter=endpoint.adapter,
                deployment=endpoint.deployment,
            )
            if default_model is None and model_info.model_name == settings.llm.default_model:
                default_model = model_info
            model_items.append(model_info)

    if default_model is None and model_items:
        default_model = model_items[0]

    return ListModelsRsp(
        code=0,
        models=model_items,
        default_model=default_model,
        providers=provider_items,
    )
