"""
模型列表 API 路由
"""
from fastapi import APIRouter

from config import settings
from harness import HarnessContainer
from observability.metrics import ai_metrics
from schemas.api import (
    DeleteApiProviderReq,
    DeleteApiProviderRsp,
    ListModelsRsp,
    ModelInfo,
    ProviderInfo,
    RegisterApiProviderReq,
    RegisterApiProviderRsp,
)

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
                supports_thinking=bool(model.get("supports_thinking", False)),
                provider_id=endpoint.provider_id,
                adapter=endpoint.adapter,
                deployment=endpoint.deployment,
            )
            if default_model is None and model_info.model_name == settings.llm.default_model:
                default_model = model_info
            model_items.append(model_info)

    if default_model is None and model_items:
        default_model = model_items[0]

    ai_metrics.http_requests.inc(route="/models", status="ok")
    return ListModelsRsp(
        code=0,
        models=model_items,
        default_model=default_model,
        providers=provider_items,
    )


@router.post("/api-provider", response_model=RegisterApiProviderRsp)
async def register_api_provider(req: RegisterApiProviderReq):
    container = HarnessContainer.get_instance()
    try:
        endpoint = await container.llm_registry.register_api_provider(
            provider_name=req.provider_name,
            base_url=req.base_url,
            api_key=req.api_key,
            adapter=req.adapter,
        )
    except Exception as exc:
        ai_metrics.http_requests.inc(route="/models/api-provider", status="error")
        return RegisterApiProviderRsp(code=400, message=str(exc))

    models = [
        ModelInfo(
            model_type=endpoint.provider_id,
            model_name=model.get("name", ""),
            display_name=model.get("display", model.get("name", "")),
            is_enabled=True,
            context_window=model.get("context_window", 0),
            supports_thinking=bool(model.get("supports_thinking", False)),
            provider_id=endpoint.provider_id,
            adapter=endpoint.adapter,
            deployment=endpoint.deployment,
        )
        for model in endpoint.models
    ]
    ai_metrics.http_requests.inc(route="/models/api-provider", status="ok")
    return RegisterApiProviderRsp(
        code=0,
        message="ok",
        provider_id=endpoint.provider_id,
        models=models,
    )


@router.post("/api-provider/delete", response_model=DeleteApiProviderRsp)
async def delete_api_provider(req: DeleteApiProviderReq):
    container = HarnessContainer.get_instance()
    try:
        deleted = container.llm_registry.delete_api_provider(req.provider_id)
    except Exception as exc:
        ai_metrics.http_requests.inc(route="/models/api-provider/delete", status="error")
        return DeleteApiProviderRsp(code=400, message=str(exc), provider_id=req.provider_id)

    if not deleted:
        ai_metrics.http_requests.inc(route="/models/api-provider/delete", status="missing")
        return DeleteApiProviderRsp(code=404, message="provider not found", provider_id=req.provider_id)

    ai_metrics.http_requests.inc(route="/models/api-provider/delete", status="ok")
    return DeleteApiProviderRsp(code=0, message="ok", provider_id=req.provider_id)
