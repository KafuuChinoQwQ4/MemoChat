"""
配置管理：从 config.yaml 加载所有配置项
使用 pydantic-settings 做类型验证
"""
import os
from pathlib import Path
from typing import Any

import yaml
from pydantic import BaseModel, Field
from pydantic_settings import BaseSettings, SettingsConfigDict


class ServiceConfig(BaseModel):
    host: str = "0.0.0.0"
    port: int = 8096


class OllamaModelConfig(BaseModel):
    name: str
    display: str
    context_window: int = 8192
    supports_thinking: bool = False


class OllamaLLMConfig(BaseModel):
    base_url: str = "http://127.0.0.1:11434"
    models: list[OllamaModelConfig] = []
    enabled: bool = True


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


class RAGConfig(BaseModel):
    qdrant: QdrantConfig = QdrantConfig()
    chunk_size: int = 500
    chunk_overlap: int = 50
    top_k: int = 5
    score_threshold: float = 0.7


class ToolsConfig(BaseModel):
    enabled: list[str] = ["duckduckgo_search", "knowledge_base", "calculator", "translator"]
    rate_limit_per_minute: int = 10


class AgentConfig(BaseModel):
    max_iterations: int = 10
    timeout_per_step_sec: int = 30
    timeout_total_sec: int = 300
    fallback_on_error: bool = True
    max_tokens_per_response: int = 2048
    temperature: float = 0.7
    hitl_enabled: bool = False
    system_prompt: str = "你是一个专业的 AI 助手。"


class Neo4jConfig(BaseModel):
    host: str = "127.0.0.1"
    port: int = 7687
    username: str = "neo4j"
    password: str = "password"
    database: str = "neo4j"
    vector_dimension: int = 384
    enabled: bool = False


class LangfuseConfig(BaseModel):
    public_key: str = ""
    secret_key: str = ""
    host: str = "http://127.0.0.1:3000"


class OtelConfig(BaseModel):
    endpoint: str = "http://127.0.0.1:4317"
    service_name: str = "ai-orchestrator"


class ObservabilityConfig(BaseModel):
    enabled: bool = False
    backend: str = "langfuse"
    langfuse: LangfuseConfig = LangfuseConfig()
    otel: OtelConfig = OtelConfig()
    trace_llm_calls: bool = True
    trace_tool_calls: bool = True
    track_ttft: bool = True


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
    episodic_limit: int = 6
    knowledge_top_k: int = 5
    providers: HarnessProvidersConfig = HarnessProvidersConfig()
    skills: HarnessSkillsConfig = HarnessSkillsConfig()


class PostgresRuntimeConfig(BaseModel):
    host: str = "127.0.0.1"
    port: int = 15432
    user: str = "memochat"
    password: str = "123456"
    database: str = "memo_pg"
    min_pool_size: int = 2
    max_pool_size: int = 10


class Settings(BaseSettings):
    model_config = SettingsConfigDict(
        env_prefix="MEMOCHAT_AI_",
        env_nested_delimiter="__",
        extra="ignore",
    )

    service: ServiceConfig = ServiceConfig()
    llm: LLMConfig = LLMConfig()
    embedding: EmbeddingConfig = EmbeddingConfig()
    rag: RAGConfig = RAGConfig()
    tools: ToolsConfig = ToolsConfig()
    agent: AgentConfig = AgentConfig()
    neo4j: Neo4jConfig = Neo4jConfig()
    observability: ObservabilityConfig = ObservabilityConfig()
    llm_fallback: LLMFallbackConfig = LLMFallbackConfig()
    mcp: MCPConfig = MCPConfig()
    harness: HarnessConfig = HarnessConfig()
    postgres: PostgresRuntimeConfig = PostgresRuntimeConfig()

    @classmethod
    def from_yaml(cls, path: str | Path) -> "Settings":
        with open(path, "r", encoding="utf-8") as f:
            raw = yaml.safe_load(f)
        return cls(**_merge_env_overrides(raw or {}))


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

    for env_key, env_value in os.environ.items():
        if not env_key.startswith(prefix):
            continue

        path = [part.lower() for part in env_key[len(prefix):].split(delimiter) if part]
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


_base_dir = Path(__file__).parent.resolve()
_cache_root = _base_dir / ".cache"

os.environ.setdefault("HF_HOME", str(_cache_root / "huggingface"))
os.environ.setdefault("SENTENCE_TRANSFORMERS_HOME", str(_cache_root / "sentence_transformers"))

_config_path = Path(os.getenv("MEMOCHAT_AI_CONFIG_PATH", str(_base_dir / "config.yaml"))).expanduser().resolve()
settings = Settings.from_yaml(_config_path)
