# Memory Layer

Purpose:
- Load short-term chat history from `ai_message`.
- Load episodic summaries from `ai_episodic_memory`.
- Load and update semantic preferences and per-user profile signals from `ai_semantic_memory`.
- Project and recall graph memory through Neo4j.

User profile:
- Stored under `preferences.user_profile`, scoped by `uid`.
- Tracks stable personalization signals such as communication style, preferred language, response format, domain interests, long-term goals, recurring constraints, likes, dislikes, and work context.
- Each signal keeps confidence, evidence count, source, and update time so repeated behavior can strengthen the profile.

Primary files:
- `service.py`: memory snapshot load/save.
- `graph_memory.py`: Neo4j projection and recall.

Coupling rule:
- Memory returns `MemorySnapshot`; prompt construction stays in orchestration.
