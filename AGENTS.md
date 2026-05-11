# MemoChat-Qml-Drogon Agent Instructions

## UI Authoring Rules

- Cross-platform UI work is compatibility-first. When fixing Linux/WSLg, macOS, Windows, Wayland/X11, DPI, font, compositor, or graphics-backend differences, preserve platforms that already work and add compatibility through platform-specific QML folders, platform-guarded C++/QML branches, resource aliases, or narrow wrapper components.
- Do not rewrite a shared QML component or replace a working platform visual path for a single-platform issue unless the bug is proven to affect that shared path. Windows Acrylic/DWM behavior, macOS behavior, and other established platform paths must remain intact unless the task explicitly targets them.
- If shared QML/C++ must be touched, keep existing defaults visually and behaviorally compatible, then add opt-in properties or platform-specific wrappers for the new platform behavior. Record the platform boundary in the plan/review and verify the changed platform plus any platform whose shared path was touched.
- Prefer additive structure for platform UI compatibility: `qml/linux`, `qml/windows`, `qml/macos`, `Qt.platform.os` checks, or `#ifdef Q_OS_*` glue. Register platform resources explicitly in `qml.qrc`; do not delete or replace older platform files while adding a new compatibility path.
- For WSLg/Linux glass and transparent rounded windows, avoid unmasked `ShaderEffectSource`/`MultiEffect`, whole-window `layer.*`, `QRegion` masks, and relying on `Rectangle.clip` for rounded child clipping. Use Linux-only antialiased QML/Shape shells with transparent insets and no square backing plates.

## Project Rules

- All infrastructure containers are in Docker.
- Docker containers must keep stable published ports.
- Project work must use the Docker containers for databases, queues, object storage, observability, and AI/RAG dependencies.
- When changing code, checking MCP, or inspecting databases, first look for the relevant Docker container or configured MCP tool.
- Default project work now targets WSL/Linux at `/root/code/MemoChat-Qml-Drogon-linux`.
- Prefer downloading Linux dependencies, caches, models, and large generated artifacts to `/data`.
- Arch native Docker is the default runtime. Use `/data/docker-data/memochat` for Docker bind data.
- Use `D:` only for Docker Desktop migration backups, legacy Windows scripts, or explicit Windows client checks.

## Skill-First Workflow

Before working on this project, read the project skills first:

1. Always read `skills/SKILL.md` for the main MemoChat task workflow.
2. Then read only the relevant focused skill files:
   - `skills/docker-diagnose.md` for Docker, fixed ports, health, or MCP startup issues.
   - `skills/db-migration.md` for Postgres, Redis, MongoDB, MinIO metadata, Neo4j, or Qdrant data changes.
   - `skills/runtime-smoke.md` for deploy/start scripts, service smoke tests, login/register/full-flow checks.
   - `skills/observability.md` for Prometheus, Loki, Tempo, Grafana, InfluxDB, cAdvisor, logs, metrics, and traces.
   - `skills/ai-rag.md` for AIOrchestrator, Ollama, Qdrant, Neo4j, MCP bridge, knowledge base, and RAG work.
   - `skills/qml-ui.md` for MemoChat QML, MemoOps QML, icons, resources, and client UI checks.
   - `skills/task.md` for normal implementation workflow.
   - `skills/withtest.md` for implementation plus iterative runtime testing.
   - `skills/planner.md` for reusable `.ai/<name>/prompt.md` and `tasks.json` automation plans.
   - `skills/parallel-agents.md` for the default Controller-led concurrency workflow used on every implementation task where safe parallel work can accelerate delivery. The Controller agent must own architecture, plan, contracts, dispatch, integration, and final acceptance; worker agents take disjoint implementation, feedback, and runtime lanes.
   - `skills/reflect.md` for learning from user corrections.
   - `skills/release.md` for release preparation and verification.
   - `skills/icon.md` for SVG/icon asset work.
3. Use `skills/PROMPTS.md` only when constructing phase prompts for delegated or artifact-based work.

Keep skill usage selective: do not load every skill file unless the task genuinely spans all of them. For implementation work, always consider the parallel workflow and record why a task stayed local-only when no worker lanes are useful.
