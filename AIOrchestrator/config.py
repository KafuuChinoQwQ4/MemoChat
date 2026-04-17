import os
from typing import Optional
from pydantic_settings import BaseSettings
from functools import lru_cache
import yaml


class Settings(BaseSettings):
    # Server
    host: str = "0.0.0.0"
    port: int = 8096

    # LLM
    default_backend: str = "ollama"
    default_model: str = "qwen2.5:7b"
    timeout_per_step_sec: int = 30
    timeout_total_sec: int = 300
    fallback_on_error: bool = True

    # Ollama
    ollama_base_url: str = "http://127.0.0.1:11434"
    ollama_enabled: bool = True

    # OpenAI
    openai_api_key: str = ""
    openai_base_url: str = "https://api.openai.com/v1"
    openai_enabled: bool = False

    # Anthropic
    anthropic_api_key: str = ""
    anthropic_base_url: str = "https://api.anthropic.com/v1"
    anthropic_enabled: bool = False

    # Kimi
    kimi_api_key: str = ""
    kimi_base_url: str = "https://api.moonshot.cn/v1"
    kimi_enabled: bool = False

    # Qdrant
    qdrant_host: str = "127.0.0.1"
    qdrant_port: int = 6333
    qdrant_grpc_port: int = 6334
    qdrant_collection_name: str = "memochat_kb"
    qdrant_enabled: bool = True

    # Neo4j
    neo4j_uri: str = "bolt://127.0.0.1:7687"
    neo4j_username: str = "neo4j"
    neo4j_password: str = "password"
    neo4j_database: str = "neo4j"
    neo4j_enabled: bool = False

    # Embedding
    embeddings_provider: str = "sentence-transformers"
    embeddings_model: str = "all-MiniLM-L6-v2"

    # Agent
    max_iterations: int = 15
    hitl_enabled: bool = True

    # Memory
    short_term_max: int = 20
    episodic_enabled: bool = True
    semantic_enabled: bool = True

    # Observability
    langfuse_public_key: str = ""
    langfuse_secret_key: str = ""
    langfuse_host: str = "http://127.0.0.1:3000"
    observability_enabled: bool = False
    otel_endpoint: str = "http://127.0.0.1:4317"

    class Config:
        env_prefix = ""
        extra = "ignore"


@lru_cache()
def get_settings() -> Settings:
    cfg_path = os.path.join(os.path.dirname(__file__), "config.yaml")
    if os.path.exists(cfg_path):
        with open(cfg_path) as f:
            yaml_cfg = yaml.safe_load(f) or {}

        flat = {}
        def flatten(d, prefix=""):
            for k, v in d.items():
                if isinstance(v, dict):
                    flatten(v, prefix + k + "_")
                else:
                    flat[prefix + k] = v
        flatten(yaml_cfg)
        return Settings(**flat)
    return Settings()


settings = get_settings()
