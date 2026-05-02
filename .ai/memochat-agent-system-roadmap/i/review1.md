# Review 1

Findings: none blocking.

Checked:
- `AgentSpecRegistry` owns five built-in templates and converts them to `AgentSkill` without replacing the existing skill path.
- `/agent/specs`, `/agent/specs/{spec_name}`, and `/agent/specs/reload` are additive API endpoints.
- Planner prompts include the selected spec policies for traceable behavior.
- `AgentHarnessService` uses spec model policy defaults only when request fields do not override them.
- Tests cover template inventory, template skill resolution, default actions, and model policy defaults.

Residual risk:
- Stage 8 templates are still single-agent templates. Crew/handoff execution is intentionally deferred to Stage 9.
