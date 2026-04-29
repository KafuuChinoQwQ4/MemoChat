# Memory Layer

Purpose:
- Load short-term chat history from `ai_message`.
- Load episodic summaries from `ai_episodic_memory`.
- Load and update semantic preferences from `ai_semantic_memory`.
- Project and recall graph memory through Neo4j.

Primary files:
- `service.py`: memory snapshot load/save.
- `graph_memory.py`: Neo4j projection and recall.

Coupling rule:
- Memory returns `MemorySnapshot`; prompt construction stays in orchestration.
