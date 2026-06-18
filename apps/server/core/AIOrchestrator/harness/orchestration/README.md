# Orchestration Layer

Purpose:
- Resolve the active skill.
- Build a bounded execution plan.
- Route agent preparation, completion, and finalization through LangGraph.
- Load memory, execute tools, call LLMs, persist trace events, and save post-turn memory through injected ports.

Primary files:
- `agent_graph.py`: LangGraph `StateGraph` runner for agent node transitions.
- `planner.py`: skill resolution helpers, plan generation, and prompt assembly.
- `agent_service.py`: node implementations and public run/stream entrypoints.

Coupling rule:
- Keep this layer dependent on `harness/ports.py` and `harness/contracts.py`.
- Add concrete construction in `harness/runtime/container.py`, not here.
- LangGraph owns control flow only; RabbitMQ, Redpanda, Postgres, Qdrant, Neo4j, FastAPI, and Qt/C++ bridge integration remain outside the graph.
