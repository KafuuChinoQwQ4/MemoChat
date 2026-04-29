# AI Agent Completion

The MemoChat AI Agent feature provides a Docker-backed AIOrchestrator with local Ollama models, Postgres trace/session storage, Neo4j graph memory, Qdrant knowledge-base RAG, and a QML Agent tab for chat, model selection, and knowledge-base operations.

The backend harness is organized by discoverable layers: orchestration, execution, feedback, memory, knowledge, LLM routing, MCP bridge, skills, and runtime assembly. Orchestration depends on lightweight ports instead of concrete service classes, trace results can be read back from persisted storage, and the layer catalog is available through `/agent/layers`.
