# Orchestration Layer

Purpose:
- Resolve the active skill.
- Build a bounded execution plan.
- Load memory, execute tools, call LLMs, persist trace events, and save post-turn memory.

Primary files:
- `planner.py`: skill resolution helpers, plan generation, and prompt assembly.
- `agent_service.py`: turn-level coordinator.

Coupling rule:
- Keep this layer dependent on `harness/ports.py` and `harness/contracts.py`.
- Add concrete construction in `harness/runtime/container.py`, not here.
