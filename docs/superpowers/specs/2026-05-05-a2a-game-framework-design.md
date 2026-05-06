# A2A Game Framework Design

Date: 2026-05-05
Project: MemoChat-Qml-Drogon
Status: Draft approved for planning

## Goal

Add a normal social game capability to the AI chat interface where a user can create a game room, invite any number of AI agents, allow human players to join, and run structured party games such as Werewolf or script murder games.

The first build should create a usable framework rather than a single hardcoded game. Werewolf is the first reference ruleset because it has clear phases, hidden roles, private/public information, voting, and agent turns. Script murder can then reuse the same room, participant, phase, event, and agent-action primitives.

## Scope For First Version

The first version builds the skeleton for A2A games inside the existing AI domain:

- Create and list game rooms from the AI chat panel.
- Join a room as a human player.
- Add multiple AI participants with model/provider settings.
- Start a room with a selected ruleset.
- Advance phases through a deterministic game engine.
- Ask agents for actions using the existing AIServer/AIOrchestrator path.
- Show public room timeline, participants, phase, and available human actions in QML.
- Persist room snapshots and events so games can be resumed or inspected.

The first version does not need full realtime multiplayer over ChatServer. It can poll room state through GateServer HTTP and later move hot room broadcasts to ChatServer/QUIC once the game loop is proven.

## Non-Goals

- No gambling, payment, adult content, or unsafe roleplay features.
- No changing Docker published ports.
- No new local database outside Docker PostgreSQL.
- No full marketplace for third-party games.
- No arbitrary code execution from rulesets; rules are compiled C++ strategies plus JSON configuration.
- No full-blown script murder content authoring tool in the first version.

## Existing Context

Relevant existing areas:

- AI UI lives under `apps/client/desktop/MemoChat-qml/qml/agent`.
- `AgentPane.qml` already hosts AI session controls, model selection, knowledge base, background tasks, and traces.
- `AgentController` already calls GateServer AI endpoints and owns model/provider state.
- GateServer AI routes are registered in `apps/server/core/GateServer/AIRouteModules.cpp`.
- AIServer exposes gRPC in `apps/server/core/common/proto/ai_message.proto` and delegates to `AIServiceCore`.
- AI trace/task persistence already exists in PostgreSQL migrations `004_ai_agent.sql` and `005_ai_agent_harness.sql`.

This feature should follow that shape: QML -> AgentController -> GateServer HTTP routes -> AIServer gRPC/core -> AIOrchestrator/model calls where needed.

## Architecture

The feature is split into five units.

### 1. Game Room API

GateServer exposes HTTP endpoints under `/ai/game/*`. These endpoints validate JSON, call AIServer through `AIServiceClient`, and return JSON to the QML client.

Initial endpoints:

- `POST /ai/game/room/create`
- `GET /ai/game/room/list?uid=...`
- `GET /ai/game/room/state?uid=...&room_id=...`
- `POST /ai/game/room/join`
- `POST /ai/game/agent/add`
- `POST /ai/game/start`
- `POST /ai/game/action`
- `POST /ai/game/tick`

`tick` is intentionally explicit in the first version. It lets tests and the UI advance agent turns without hiding behavior in a long-running background loop. A later version can add scheduled ticks or event bus consumers.

### 2. AIServer Game Core

AIServer owns the canonical room state. The new core code should live under `apps/server/core/AIServer/game`.

Proposed files:

- `A2AGameTypes.h`: enums and structs for room, participant, phase, action, event, visibility.
- `A2AGameJsonMapper.h/.cpp`: conversion between structs and JSON/protobuf string payloads.
- `A2AGameRuleEngine.h`: interface for rulesets.
- `WerewolfRuleEngine.h/.cpp`: first concrete ruleset, minimal and deterministic.
- `A2AGameAgentRunner.h/.cpp`: builds prompts for AI participants and parses structured actions.
- `A2AGameService.h/.cpp`: room lifecycle, action validation, phase transition, persistence calls.
- `db/A2AGameRepo.h/.cpp`: PostgreSQL persistence for rooms and events.

`AIServiceCore` should delegate game RPC handlers to `A2AGameService`, keeping the main AI service file from becoming the rules engine.

### 3. Ruleset Contract

A ruleset receives a current room state and an action, then returns a new state plus events.

Core methods:

- `ruleset_id()` returns stable id such as `werewolf.basic`.
- `initial_state(config, participants)` creates roles, phase, round, and visibility.
- `available_actions(state, participant_id)` returns actions visible to a participant.
- `apply_action(state, action)` validates and mutates state.
- `next_pending_actor(state)` identifies the next human/agent who must act.
- `public_view(state, uid)` hides secret roles and private notes from other players.

For Werewolf first pass, use a small ruleset:

- Roles: villager, werewolf, seer, witch can be represented, but only villager/werewolf need complete resolution in the first pass.
- Phases: lobby, night, day_discussion, vote, ended.
- Actions: speak, vote, night_kill, skip.
- Victory: werewolves eliminated or wolves >= villagers.

Script murder later uses the same contract with phases such as intro, free_talk, clue, accuse, reveal.

### 4. Agent Runner

AI participants are normal game participants with `kind = agent`. Each has:

- display name
- model type/name
- persona prompt
- strategy level
- private memory JSON

When the engine needs an agent action, `A2AGameAgentRunner` constructs a prompt containing:

- ruleset summary
- public timeline
- private role/knowledge for that agent
- allowed action schema
- recent messages

The response must be parsed as JSON. If parsing fails, the runner emits a public `agent_error` event and returns `skip` for the first version. This keeps the room from getting stuck.

### 5. QML Client

Add a game surface inside the AI chat interface instead of creating a separate top-level app.

Proposed QML files:

- `qml/agent/game/A2AGamePane.qml`: main panel with room list, active room, timeline, and actions.
- `qml/agent/game/GameRoomList.qml`: room cards and create button.
- `qml/agent/game/GameRoomHeader.qml`: ruleset, phase, round, status.
- `qml/agent/game/GameParticipantList.qml`: humans and agents.
- `qml/agent/game/GameTimeline.qml`: public events and speeches.
- `qml/agent/game/GameActionBar.qml`: speak/vote/skip/start/tick controls.
- `qml/agent/game/CreateGameRoomDialog.qml`: ruleset, title, max players, agent count.

`AgentPane.qml` should add a compact header action named `游戏` and load the game pane as a side tool or tab. The first implementation can use polling from `AgentController` every few seconds when a room is active.

## Data Model

PostgreSQL tables:

### `ai_game_room`

- `room_id VARCHAR(64) PRIMARY KEY`
- `owner_uid INTEGER NOT NULL`
- `title VARCHAR(128) NOT NULL`
- `ruleset_id VARCHAR(64) NOT NULL`
- `status VARCHAR(32) NOT NULL`
- `phase VARCHAR(64) NOT NULL`
- `round_index INTEGER NOT NULL DEFAULT 0`
- `max_players INTEGER NOT NULL DEFAULT 12`
- `state_json JSONB NOT NULL DEFAULT '{}'::jsonb`
- `created_at BIGINT NOT NULL`
- `updated_at BIGINT NOT NULL`
- `ended_at BIGINT DEFAULT NULL`
- `deleted_at BIGINT DEFAULT NULL`

### `ai_game_participant`

- `participant_id VARCHAR(64) PRIMARY KEY`
- `room_id VARCHAR(64) NOT NULL`
- `kind VARCHAR(16) NOT NULL` where values are `human` or `agent`
- `uid INTEGER DEFAULT NULL`
- `display_name VARCHAR(80) NOT NULL`
- `model_type VARCHAR(64) NOT NULL DEFAULT ''`
- `model_name VARCHAR(128) NOT NULL DEFAULT ''`
- `persona TEXT NOT NULL DEFAULT ''`
- `role_key VARCHAR(64) NOT NULL DEFAULT ''`
- `status VARCHAR(32) NOT NULL DEFAULT 'alive'`
- `private_state_json JSONB NOT NULL DEFAULT '{}'::jsonb`
- `created_at BIGINT NOT NULL`
- `updated_at BIGINT NOT NULL`

### `ai_game_event`

- `event_id VARCHAR(64) PRIMARY KEY`
- `room_id VARCHAR(64) NOT NULL`
- `round_index INTEGER NOT NULL`
- `phase VARCHAR(64) NOT NULL`
- `visibility VARCHAR(32) NOT NULL DEFAULT 'public'`
- `actor_participant_id VARCHAR(64) NOT NULL DEFAULT ''`
- `event_type VARCHAR(64) NOT NULL`
- `content TEXT NOT NULL DEFAULT ''`
- `payload_json JSONB NOT NULL DEFAULT '{}'::jsonb`
- `created_at BIGINT NOT NULL`

Keep the full mutable state in `state_json` during the first version. Normalize only the room, participant, and event surfaces needed by UI and audit. If querying becomes important later, add indexes or extracted columns.

## API Payloads

Create room:

```json
{
  "uid": 1001,
  "title": "今晚狼人杀",
  "ruleset_id": "werewolf.basic",
  "max_players": 8,
  "config": {
    "agent_count": 5,
    "human_slots": 3
  }
}
```

Add agent:

```json
{
  "uid": 1001,
  "room_id": "room-uuid",
  "display_name": "冷静分析师",
  "model_type": "ollama",
  "model_name": "qwen3:4b",
  "persona": "发言简洁，优先找逻辑矛盾。"
}
```

Submit action:

```json
{
  "uid": 1001,
  "room_id": "room-uuid",
  "participant_id": "participant-uuid",
  "action_type": "speak",
  "target_participant_id": "",
  "content": "我先听一轮发言。",
  "payload": {}
}
```

State response:

```json
{
  "code": 0,
  "room": {
    "room_id": "room-uuid",
    "title": "今晚狼人杀",
    "ruleset_id": "werewolf.basic",
    "status": "running",
    "phase": "day_discussion",
    "round_index": 1
  },
  "participants": [],
  "events": [],
  "available_actions": []
}
```

## Safety And Moderation

This is a normal social-game feature. Keep the first version bounded:

- Rulesets are known and server-side.
- Agent prompts explicitly require game-only roleplay and no harmful real-world instructions.
- Secret role information is only included in the acting participant's private prompt.
- Public state filters private roles and private actions.
- Room owner can end a room.
- Invalid action attempts are logged as private system events.

## Observability

Use existing logging patterns:

- Gate route spans: `gate.ai.game.*`
- AIServer spans: `AIService.Game*`
- Game service events: room created, participant joined, game started, action applied, agent action failed, game ended.

Later, game events can also emit Redpanda business events, but the first version can rely on PostgreSQL audit rows and service logs.

## Verification Strategy

Use narrow tests first, then full build:

- Unit test JSON mapping for game payloads.
- Unit test Werewolf phase transitions and victory checks without network.
- Unit test action validation for invalid actor/phase/target.
- Build with `cmake --preset msvc2022-full` and `cmake --build --preset msvc2022-full` before runtime verification.
- Runtime smoke through GateServer endpoints after deploy.

Docker/PostgreSQL must be used for migration and runtime checks. Do not change stable Docker ports.

## Open Decisions

- Whether room state should eventually be broadcast over ChatServer long connections or remain HTTP-polling for AI games.
- Whether script murder content is authored as JSON scenario files, database records, or knowledge-base documents.
- Whether agents should be spawned as background tasks or synchronous tick calls. First version uses explicit tick calls for determinism.
