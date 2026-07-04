"""
配置管理：从 config.yaml 加载所有配置项
使用 pydantic-settings 做类型验证
"""

import ipaddress
import os
from pathlib import Path
from typing import Any

import yaml
from pydantic import BaseModel, Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class ServiceConfig(BaseModel):
    host: str = "0.0.0.0"
    port: int = 8096


class SecurityConfig(BaseModel):
    enforce_internal_auth: bool = True
    internal_auth_header: str = "X-MemoChat-AI-Internal-Key"
    internal_api_key: str = ""
    internal_api_key_env: str = "MEMOCHAT_AI_INTERNAL_API_KEY"
    provider_admin_auth_header: str = "X-MemoChat-AI-Provider-Admin-Key"
    provider_admin_key: str = ""
    provider_admin_key_env: str = "MEMOCHAT_AI_PROVIDER_ADMIN_KEY"
    trusted_client_hosts: list[str] = []
    unauthenticated_paths: list[str] = ["/health", "/ready", "/metrics"]
    cors_allow_origins: list[str] = ["http://127.0.0.1", "http://localhost"]
    cors_allow_origin_regex: str = r"^https?://(127\.0\.0\.1|localhost)(:\d+)?$"


class OllamaModelConfig(BaseModel):
    name: str
    display: str
    context_window: int = 8192
    supports_thinking: bool = False


class OllamaLLMConfig(BaseModel):
    base_url: str = "http://127.0.0.1:11434"
    models: list[OllamaModelConfig] = []
    enabled: bool = True
    timeout_sec: int = 300


class OpenAIModelConfig(BaseModel):
    name: str
    display: str
    context_window: int = 128000
    supports_thinking: bool = False


class OpenAILLMConfig(BaseModel):
    api_key: str = ""
    base_url: str = "https://api.openai.com/v1"
    models: list[OpenAIModelConfig] = []
    enabled: bool = False


class AnthropicModelConfig(BaseModel):
    name: str
    display: str
    context_window: int = 200000
    supports_thinking: bool = False


class AnthropicLLMConfig(BaseModel):
    api_key: str = ""
    base_url: str = "https://api.anthropic.com/v1"
    models: list[AnthropicModelConfig] = []
    enabled: bool = False


class KimiModelConfig(BaseModel):
    name: str
    display: str
    context_window: int = 8192
    supports_thinking: bool = False


class KimiLLMConfig(BaseModel):
    api_key: str = ""
    base_url: str = "https://api.moonshot.cn/v1"
    models: list[KimiModelConfig] = []
    enabled: bool = False


class LLMConfig(BaseModel):
    default_backend: str = "ollama"
    default_model: str = "qwen3:4b"
    ollama: OllamaLLMConfig = OllamaLLMConfig()
    openai: OpenAILLMConfig = OpenAILLMConfig()
    anthropic: AnthropicLLMConfig = AnthropicLLMConfig()
    kimi: KimiLLMConfig = KimiLLMConfig()


class EmbeddingConfig(BaseModel):
    backend: str = "local"
    local_model: str = "all-MiniLM-L6-v2"
    ollama_model: str = "nomic-embed-text"
    openai_model: str = "text-embedding-3-small"
    dimension: int = 384


class QdrantConfig(BaseModel):
    host: str = "127.0.0.1"
    port: int = 6333
    grpc_port: int = 6334
    collection_prefix: str = "user_"


class RAGRetrievalConfig(BaseModel):
    dense_top_k: int = 12
    lexical_top_k: int = 12
    candidate_pool_size: int = 24
    rrf_k: int = 60
    dense_weight: float = 1.0
    lexical_weight: float = 1.0
    rerank_enabled: bool = True
    rerank_model: str = "BAAI/bge-reranker-base"
    rerank_batch_size: int = 8
    rerank_candidate_limit: int = 16


class RAGConfig(BaseModel):
    qdrant: QdrantConfig = QdrantConfig()
    retrieval: RAGRetrievalConfig = RAGRetrievalConfig()
    chunk_size: int = 500
    chunk_overlap: int = 50
    top_k: int = 5
    score_threshold: float = 0.7


class ToolsConfig(BaseModel):
    enabled: list[str] = ["web_search", "knowledge_base", "calculator", "translator"]
    rate_limit_per_minute: int = 10


class AgentConfig(BaseModel):
    max_iterations: int = 10
    timeout_per_step_sec: int = 30
    timeout_total_sec: int = 300
    fallback_on_error: bool = True
    max_tokens_per_response: int = 8192
    temperature: float = 0.7
    hitl_enabled: bool = False
    system_prompt: str = "你是一个专业的 AI 助手。"


class Neo4jConfig(BaseModel):
    host: str = "127.0.0.1"
    port: int = 7687
    username: str = "neo4j"
    password: str = ""
    database: str = "neo4j"
    vector_dimension: int = 384
    enabled: bool = False


class LangSmithConfig(BaseModel):
    tracing: bool = True
    endpoint: str = "https://api.smith.langchain.com"
    api_key_env: str = "LANGSMITH_API_KEY"
    project: str = "memochat-ai-local"
    sampling_rate: float = 1.0
    redact_inputs: bool = True
    redact_outputs: bool = False
    upload_user_content: bool = False
    tags: list[str] = ["memochat", "ai-agent", "local"]


class OtelConfig(BaseModel):
    endpoint: str = "http://127.0.0.1:4317"
    service_name: str = "ai-orchestrator"


class ObservabilityConfig(BaseModel):
    enabled: bool = False
    backend: str = "langsmith"
    langsmith: LangSmithConfig = LangSmithConfig()
    otel: OtelConfig = OtelConfig()
    prometheus_enabled: bool = True
    trace_llm_calls: bool = True
    trace_tool_calls: bool = True
    track_ttft: bool = True


class RedpandaQueueConfig(BaseModel):
    enabled: bool = True
    bootstrap_servers: str = "127.0.0.1:19092"
    proxy_url: str = "http://127.0.0.1:18082"
    proxy_fallback_enabled: bool = True
    client_id: str = "memochat-ai-orchestrator"
    task_events_topic: str = "ai.agent.task.events.v1"
    publish_timeout_sec: float = 2.0


class RabbitMQQueueConfig(BaseModel):
    enabled: bool = True
    host: str = "127.0.0.1"
    port: int = 5672
    username: str = "memochat"
    password: str = ""
    vhost: str = "/"
    exchange: str = "memochat.direct"
    dlx_exchange: str = "memochat.dlx"
    task_queue: str = "ai.agent.tasks.q"
    task_routing_key: str = "ai.agent.task.run"
    prefetch_count: int = 16
    publish_timeout_sec: float = 2.0


class AgentQueueConfig(BaseModel):
    enabled: bool = False
    backend: str = "hybrid"
    worker_concurrency: int = 8
    fallback_to_local: bool = True
    resume_stale_on_start: bool = True
    rabbitmq: RabbitMQQueueConfig = RabbitMQQueueConfig()
    redpanda: RedpandaQueueConfig = RedpandaQueueConfig()


class SemanticCacheRedisConfig(BaseModel):
    host: str = "127.0.0.1"
    port: int = 6379
    username: str = ""
    password: str = ""
    db: int = 0
    ssl: bool = False
    socket_timeout_sec: float = 2.0
    connect_timeout_sec: float = 1.0


class SemanticCacheConfig(BaseModel):
    enabled: bool = True
    exact_match_enabled: bool = True
    redis: SemanticCacheRedisConfig = SemanticCacheRedisConfig()
    index_name: str = "memochat_ai_semantic_cache_v1"
    key_prefix: str = "memochat:ai:semantic_cache:"
    scope: str = "user"
    similarity_threshold: float = 0.98
    top_k: int = 1
    ttl_sec: int = 7 * 24 * 60 * 60
    min_question_chars: int = 4
    max_question_chars: int = 2000
    max_answer_chars: int = 32000
    dimension: int = 384
    include_model_in_cache_key: bool = True
    cache_tool_results: bool = False
    persist_hits_to_memory: bool = True
    skip_volatile_inputs: bool = True
    non_cacheable_actions: list[str] = ["web_search", "mcp_tool", "graph_recall"]


class LLMFallbackConfig(BaseModel):
    ollama: list[str] = ["openai", "kimi"]
    openai: list[str] = ["kimi"]
    claude: list[str] = ["openai"]
    kimi: list[str] = ["openai"]
    retry_count: int = 2
    retry_delay_sec: int = 2


class MCPServerConfig(BaseModel):
    name: str
    command: list[str]
    args: list[str] = []
    env: dict[str, str] = {}
    allowed_tools: list[str] = []
    enabled: bool = True


class MCPConfig(BaseModel):
    enabled: bool = False
    servers: list[MCPServerConfig] = []


class HarnessModelConfig(BaseModel):
    name: str
    display: str
    context_window: int = 128000
    supports_thinking: bool = False


class HarnessEndpointConfig(BaseModel):
    name: str
    adapter: str = "openai_compatible"
    deployment: str = "external_api"
    base_url: str = ""
    api_key: str = ""
    api_key_env: str = ""
    default_model: str = ""
    enabled: bool = False
    timeout_sec: int = 120
    thinking_parameter: str = ""
    models: list[HarnessModelConfig] = []


class HarnessProvidersConfig(BaseModel):
    endpoints: list[HarnessEndpointConfig] = []


class HarnessSkillsConfig(BaseModel):
    enabled: list[str] = [
        "general_chat",
        "knowledge_copilot",
        "research_assistant",
        "summarize_thread",
        "reply_suggester",
        "translate_text",
        "graph_recommender",
        "mcp_operator",
    ]


class HarnessConfig(BaseModel):
    enabled: bool = True
    canonical_root: str = "server/AIOrchestrator/harness"
    industrial_target_year: int = 2026
    default_mode: str = "agent"
    max_planning_steps: int = 6
    max_tool_rounds: int = 4
    trace_persist: bool = True
    auto_reflection: bool = True
    short_term_limit: int = 24
    short_term_fetch_limit: int = 96
    short_term_token_budget: int = 1600
    short_term_summary_token_budget: int = 320
    episodic_limit: int = 6
    knowledge_top_k: int = 5
    providers: HarnessProvidersConfig = HarnessProvidersConfig()
    skills: HarnessSkillsConfig = HarnessSkillsConfig()


class PostgresRuntimeConfig(BaseModel):
    host: str = "127.0.0.1"
    port: int = 15432
    user: str = "memochat"
    password: str = ""
    database: str = "memo_pg"
    min_pool_size: int = 2
    max_pool_size: int = 10


class PetFeatureConfig(BaseModel):
    enabled: bool = True
    deterministic: bool = True
    live2d_native_enabled: bool = False
    live2d_sdk_root: str = ""
    asset_root: str = ""
    cloud_vision_enabled: bool = False
    local_vision_enabled: bool = False
    vision_camera_index: int = 0
    vision_analyzer: str = "opencv"
    face_landmarker_model_path: str = ""
    object_detector_model_path: str = ""
    vision_retain_raw_frames: bool = False
    voice_clone_enabled: bool = False
    voice_provider: str = "scripted"
    voice_sovits_base_url: str = ""
    voice_sovits_reference_audio: str = ""
    voice_sovits_prompt_text: str = ""
    voice_sovits_prompt_language: str = "zh"
    voice_sovits_text_language: str = "zh"
    voice_sovits_output_dir: str = "/app/.data/pet-voice-cache"
    voice_sovits_timeout_sec: float = 180.0
    pet_text_timeout_sec: float = 25.0
    voice_training_enabled: bool = True
    voice_training_artifact_root: str = "/app/.data/pet-voice-training"


class Settings(BaseSettings):
    model_config = SettingsConfigDict(
        env_prefix="MEMOCHAT_AI_",
        env_nested_delimiter="__",
        extra="ignore",
    )

    service: ServiceConfig = ServiceConfig()
    security: SecurityConfig = SecurityConfig()
    llm: LLMConfig = LLMConfig()
    embedding: EmbeddingConfig = EmbeddingConfig()
    rag: RAGConfig = RAGConfig()
    tools: ToolsConfig = ToolsConfig()
    agent: AgentConfig = AgentConfig()
    neo4j: Neo4jConfig = Neo4jConfig()
    observability: ObservabilityConfig = ObservabilityConfig()
    agent_queue: AgentQueueConfig = AgentQueueConfig()
    semantic_cache: SemanticCacheConfig = SemanticCacheConfig()
    llm_fallback: LLMFallbackConfig = LLMFallbackConfig()
    mcp: MCPConfig = MCPConfig()
    harness: HarnessConfig = HarnessConfig()
    postgres: PostgresRuntimeConfig = PostgresRuntimeConfig()
    pet: PetFeatureConfig = PetFeatureConfig()

    @classmethod
    def from_yaml(cls, path: str | Path) -> "Settings":
        with open(path, "r", encoding="utf-8") as f:
            raw = yaml.safe_load(f)
        return cls(**_merge_env_overrides(raw or {}))


def resolve_internal_api_key(security: SecurityConfig) -> str:
    env_name = (security.internal_api_key_env or "").strip()
    if env_name:
        env_value = os.getenv(env_name, "").strip()
        if env_value:
            return env_value
    return (security.internal_api_key or "").strip()


def resolve_provider_admin_key(security: SecurityConfig) -> str:
    env_name = (security.provider_admin_key_env or "").strip()
    if env_name:
        env_value = os.getenv(env_name, "").strip()
        if env_value:
            return env_value
    return (security.provider_admin_key or "").strip()


def is_unauthenticated_path(path: str, allowed_paths: list[str]) -> bool:
    normalized = (path or "/").rstrip("/") or "/"
    for allowed in allowed_paths:
        candidate = (allowed or "").rstrip("/") or "/"
        if normalized == candidate:
            return True
    return False


def is_trusted_client_host(client_host: str, trusted_entries: list[str]) -> bool:
    candidate = (client_host or "").strip()
    if not candidate:
        return False
    if "%" in candidate:
        candidate = candidate.split("%", 1)[0]

    try:
        candidate_ip = ipaddress.ip_address(candidate)
    except ValueError:
        candidate_ip = None

    for raw_entry in trusted_entries:
        entry = (raw_entry or "").strip()
        if not entry:
            continue
        if candidate_ip is None:
            if candidate.lower() == entry.lower():
                return True
            continue
        try:
            if "/" in entry:
                if candidate_ip in ipaddress.ip_network(entry, strict=False):
                    return True
            elif candidate_ip == ipaddress.ip_address(entry):
                return True
        except ValueError:
            if candidate.lower() == entry.lower():
                return True
    return False


def _parse_env_value(value: str) -> Any:
    stripped = value.strip()
    if stripped.lower() in {"true", "false"}:
        return stripped.lower() == "true"
    if stripped.lower() in {"null", "none"}:
        return None
    if stripped.startswith(("{", "[")):
        try:
            return yaml.safe_load(stripped)
        except Exception:
            return value
    return value


def _merge_env_overrides(raw: dict[str, Any]) -> dict[str, Any]:
    merged = dict(raw)
    prefix = "MEMOCHAT_AI_"
    delimiter = "__"

    _merge_pet_alias_env(merged)

    for env_key, env_value in os.environ.items():
        if not env_key.startswith(prefix):
            continue

        path = [part.lower() for part in env_key[len(prefix) :].split(delimiter) if part]
        if not path:
            continue

        cursor = merged
        for part in path[:-1]:
            next_value = cursor.get(part)
            if not isinstance(next_value, dict):
                next_value = {}
                cursor[part] = next_value
            cursor = next_value
        cursor[path[-1]] = _parse_env_value(env_value)

    return merged


_PET_ENV_ALIASES = {
    "MEMOCHAT_ENABLE_PET": "enabled",
    "MEMOCHAT_PET_DETERMINISTIC": "deterministic",
    "MEMOCHAT_ENABLE_LIVE2D_NATIVE": "live2d_native_enabled",
    "MEMOCHAT_LIVE2D_SDK_ROOT": "live2d_sdk_root",
    "MEMOCHAT_PET_ASSET_ROOT": "asset_root",
    "MEMOCHAT_PET_CLOUD_VISION": "cloud_vision_enabled",
    "MEMOCHAT_PET_LOCAL_VISION": "local_vision_enabled",
    "MEMOCHAT_PET_VISION_CAMERA_INDEX": "vision_camera_index",
    "MEMOCHAT_PET_VISION_ANALYZER": "vision_analyzer",
    "MEMOCHAT_PET_FACE_LANDMARKER_MODEL": "face_landmarker_model_path",
    "MEMOCHAT_PET_OBJECT_DETECTOR_MODEL": "object_detector_model_path",
    "MEMOCHAT_PET_VISION_RETAIN_RAW_FRAMES": "vision_retain_raw_frames",
    "MEMOCHAT_PET_VOICE_CLONE": "voice_clone_enabled",
    "MEMOCHAT_PET_VOICE_PROVIDER": "voice_provider",
    "MEMOCHAT_PET_SOVITS_BASE_URL": "voice_sovits_base_url",
    "MEMOCHAT_PET_SOVITS_REFERENCE_AUDIO": "voice_sovits_reference_audio",
    "MEMOCHAT_PET_SOVITS_PROMPT_TEXT": "voice_sovits_prompt_text",
    "MEMOCHAT_PET_SOVITS_PROMPT_LANGUAGE": "voice_sovits_prompt_language",
    "MEMOCHAT_PET_SOVITS_TEXT_LANGUAGE": "voice_sovits_text_language",
    "MEMOCHAT_PET_SOVITS_OUTPUT_DIR": "voice_sovits_output_dir",
    "MEMOCHAT_PET_SOVITS_TIMEOUT_SEC": "voice_sovits_timeout_sec",
    "MEMOCHAT_PET_TEXT_TIMEOUT_SEC": "pet_text_timeout_sec",
    "MEMOCHAT_PET_VOICE_TRAINING": "voice_training_enabled",
    "MEMOCHAT_PET_VOICE_TRAINING_ARTIFACT_ROOT": "voice_training_artifact_root",
}


def _merge_pet_alias_env(merged: dict[str, Any]) -> None:
    pet_config = merged.get("pet")
    if not isinstance(pet_config, dict):
        pet_config = {}
        merged["pet"] = pet_config

    for env_key, field_name in _PET_ENV_ALIASES.items():
        if env_key in os.environ:
            pet_config[field_name] = _parse_env_value(os.environ[env_key])


_base_dir = Path(__file__).parent.resolve()
_cache_root = _base_dir / ".cache"

os.environ.setdefault("HF_HOME", str(_cache_root / "huggingface"))
os.environ.setdefault("SENTENCE_TRANSFORMERS_HOME", str(_cache_root / "sentence_transformers"))

_config_path = Path(os.getenv("MEMOCHAT_AI_CONFIG_PATH", str(_base_dir / "config.yaml"))).expanduser().resolve()
settings = Settings.from_yaml(_config_path)
