# A2A Game Framework Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first A2A game framework inside MemoChat's AI chat interface so humans and multiple AI agents can join structured social games.

**Architecture:** The feature extends the existing AI path: QML `AgentPane` -> `AgentController` -> GateServer `/ai/game/*` HTTP routes -> AIServer gRPC -> `A2AGameService`. AIServer owns canonical state, PostgreSQL stores rooms/events, and rulesets are compiled C++ engines behind a small interface.

**Tech Stack:** Qt/QML, C++23, Boost/Beast GateServer routing, gRPC/protobuf, PostgreSQL JSONB migrations, existing AIServer AI client/orchestrator path, CMake `msvc2022-full`.

---

## File Structure

Create server game core:

- `apps/server/core/AIServer/game/A2AGameTypes.h`: room, participant, action, event, and state structs.
- `apps/server/core/AIServer/game/A2AGameJsonMapper.h`
- `apps/server/core/AIServer/game/A2AGameJsonMapper.cpp`: convert JSON strings and structs for HTTP/protobuf payloads.
- `apps/server/core/AIServer/game/A2AGameRuleEngine.h`: ruleset interface.
- `apps/server/core/AIServer/game/WerewolfRuleEngine.h`
- `apps/server/core/AIServer/game/WerewolfRuleEngine.cpp`: first minimal ruleset.
- `apps/server/core/AIServer/game/A2AGameAgentRunner.h`
- `apps/server/core/AIServer/game/A2AGameAgentRunner.cpp`: build agent prompts and parse JSON actions.
- `apps/server/core/AIServer/game/A2AGameService.h`
- `apps/server/core/AIServer/game/A2AGameService.cpp`: room lifecycle and orchestration.
- `apps/server/core/AIServer/game/db/A2AGameRepo.h`
- `apps/server/core/AIServer/game/db/A2AGameRepo.cpp`: PostgreSQL persistence.

Modify server API:

- `apps/server/core/common/proto/ai_message.proto`: add game request/response messages and AIService RPCs.
- `apps/server/core/AIServer/AIServiceCore.h`
- `apps/server/core/AIServer/AIServiceCore.cpp`: delegate game RPCs to `A2AGameService`.
- `apps/server/core/AIServer/AIServiceImpl.h`
- `apps/server/core/AIServer/AIServiceImpl.cpp`: expose RPC methods.
- `apps/server/core/AIServer/CMakeLists.txt`: compile new game files.
- `apps/server/core/GateServer/AIServiceClient.h`
- `apps/server/core/GateServer/AIServiceClient.cpp`: call game RPCs.
- `apps/server/core/GateServer/AIRouteModules.cpp`: register `/ai/game/*` routes.

Create migration:

- `apps/server/migrations/postgresql/business/006_ai_game_framework.sql`: create `ai_game_room`, `ai_game_participant`, `ai_game_event`.

Create client UI and controller support:

- `apps/client/desktop/MemoChat-qml/AgentController.h`: add game properties and invokable methods.
- `apps/client/desktop/MemoChat-qml/AgentController.cpp`: call GateServer game routes and update QML state.
- `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`: add game entry and load game pane.
- `apps/client/desktop/MemoChat-qml/qml/agent/game/A2AGamePane.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/game/GameRoomList.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/game/GameRoomHeader.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/game/GameParticipantList.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/game/GameTimeline.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/game/GameActionBar.qml`
- `apps/client/desktop/MemoChat-qml/qml/agent/game/CreateGameRoomDialog.qml`
- `apps/client/desktop/MemoChat-qml/qml.qrc`: register new QML files.

Create tests:

- `apps/server/core/AIServer/tests/A2AGameJsonMapperTest.cpp`
- `apps/server/core/AIServer/tests/WerewolfRuleEngineTest.cpp`
- `apps/server/core/AIServer/tests/A2AGameServiceTest.cpp`
- Update relevant test CMake under the AIServer/test area used by existing `AIServiceJsonMapperTest.cpp`.

---

### Task 1: Add Persistent Schema

**Files:**

- Create: `apps/server/migrations/postgresql/business/006_ai_game_framework.sql`

- [ ] **Step 1: Create the migration file**

Add the three tables exactly named `ai_game_room`, `ai_game_participant`, and `ai_game_event`. Use JSONB defaults and indexes from the design spec.

- [ ] **Step 2: Add indexes**

Add indexes for active room lookup, participant lookup by room, and event timeline lookup:

```sql
CREATE INDEX IF NOT EXISTS idx_ai_game_room_owner_updated
    ON ai_game_room (owner_uid, updated_at DESC)
    WHERE deleted_at IS NULL;

CREATE INDEX IF NOT EXISTS idx_ai_game_participant_room
    ON ai_game_participant (room_id, created_at ASC);

CREATE INDEX IF NOT EXISTS idx_ai_game_event_room_created
    ON ai_game_event (room_id, created_at ASC);
```

- [ ] **Step 3: Verify migration syntax in Docker PostgreSQL**

Run:

```powershell
docker exec memochat-postgres psql -U memochat -d memo_pg -f /repo/apps/server/migrations/postgresql/business/006_ai_game_framework.sql
```

If `/repo` is not mounted in the container, copy the SQL into a temporary container file under `/tmp` and execute that file. Expected result: `CREATE TABLE` and `CREATE INDEX` messages without SQL errors.

---

### Task 2: Define Game Domain Types And JSON Mapping

**Files:**

- Create: `apps/server/core/AIServer/game/A2AGameTypes.h`
- Create: `apps/server/core/AIServer/game/A2AGameJsonMapper.h`
- Create: `apps/server/core/AIServer/game/A2AGameJsonMapper.cpp`
- Test: `apps/server/core/AIServer/tests/A2AGameJsonMapperTest.cpp`

- [ ] **Step 1: Define C++ structs**

Create structs for `A2AGameRoom`, `A2AGameParticipant`, `A2AGameEvent`, `A2AGameAction`, and `A2AGameStateView`. Use `std::string`, `int32_t`, `int64_t`, and `std::vector` only.

- [ ] **Step 2: Implement JSON mapper methods**

Expose these methods:

```cpp
class A2AGameJsonMapper {
public:
    static std::optional<A2AGameAction> ParseAction(const std::string& json, std::string* error);
    static std::string WriteStateView(const A2AGameStateView& view);
    static std::string WriteRoom(const A2AGameRoom& room);
    static std::string WriteParticipants(const std::vector<A2AGameParticipant>& participants);
    static std::string WriteEvents(const std::vector<A2AGameEvent>& events);
};
```

Use the existing `memochat::json` helpers already used by `AIRouteModules.cpp`.

- [ ] **Step 3: Add mapper tests**

Test parsing a valid speak action, rejecting an empty `room_id`, and serializing a state view with one participant and one event.

- [ ] **Step 4: Run focused mapper test**

Run the configured AIServer test target once it is wired into CMake. Expected: mapper test passes and invalid action returns a non-empty error string.

---

### Task 3: Add Ruleset Interface And Minimal Werewolf Engine

**Files:**

- Create: `apps/server/core/AIServer/game/A2AGameRuleEngine.h`
- Create: `apps/server/core/AIServer/game/WerewolfRuleEngine.h`
- Create: `apps/server/core/AIServer/game/WerewolfRuleEngine.cpp`
- Test: `apps/server/core/AIServer/tests/WerewolfRuleEngineTest.cpp`

- [ ] **Step 1: Define the ruleset interface**

Add an abstract class with these methods:

```cpp
class A2AGameRuleEngine {
public:
    virtual ~A2AGameRuleEngine() = default;
    virtual std::string RulesetId() const = 0;
    virtual A2AGameStateView CreateInitialState(const A2AGameRoom& room,
                                                const std::vector<A2AGameParticipant>& participants,
                                                const std::string& config_json) const = 0;
    virtual std::vector<std::string> AvailableActions(const A2AGameStateView& state,
                                                      const std::string& participant_id) const = 0;
    virtual bool ApplyAction(A2AGameStateView* state,
                             const A2AGameAction& action,
                             std::vector<A2AGameEvent>* events,
                             std::string* error) const = 0;
    virtual std::optional<std::string> NextPendingActor(const A2AGameStateView& state) const = 0;
};
```

- [ ] **Step 2: Implement `werewolf.basic` phases**

Support `lobby`, `night`, `day_discussion`, `vote`, and `ended`. First version actions are `start`, `speak`, `night_kill`, `vote`, and `skip`.

- [ ] **Step 3: Keep role assignment deterministic for tests**

When the first version starts, assign the first participant as `werewolf` and all others as `villager` unless `config_json` explicitly contains role assignments.

- [ ] **Step 4: Add ruleset tests**

Cover start from lobby, speak during day discussion, reject vote during night, night kill advances to day discussion, and victory when werewolf count is zero.

- [ ] **Step 5: Run focused ruleset tests**

Expected: all ruleset tests pass without network or database dependencies.

---

### Task 4: Add Game Repository

**Files:**

- Create: `apps/server/core/AIServer/game/db/A2AGameRepo.h`
- Create: `apps/server/core/AIServer/game/db/A2AGameRepo.cpp`

- [ ] **Step 1: Follow existing AIServer repo style**

Mirror connection and query patterns from `apps/server/core/AIServer/db/AISessionRepo.cpp` and `AISmartLogRepo.cpp`.

- [ ] **Step 2: Implement room methods**

Add `CreateRoom`, `UpdateRoomState`, `GetRoom`, `ListRoomsForUid`, and `SoftDeleteRoom`.

- [ ] **Step 3: Implement participant methods**

Add `AddParticipant`, `ListParticipants`, and `UpdateParticipantState`.

- [ ] **Step 4: Implement event methods**

Add `AppendEvent` and `ListEvents(room_id, limit, offset)` ordered by `created_at ASC`.

- [ ] **Step 5: Verify with Docker PostgreSQL**

Use the existing runtime config and run a small service-level test or manual psql probe to confirm inserted rows appear in `ai_game_room` and `ai_game_event`.

---

### Task 5: Add AIServer Game Service

**Files:**

- Create: `apps/server/core/AIServer/game/A2AGameService.h`
- Create: `apps/server/core/AIServer/game/A2AGameService.cpp`
- Create: `apps/server/core/AIServer/game/A2AGameAgentRunner.h`
- Create: `apps/server/core/AIServer/game/A2AGameAgentRunner.cpp`
- Test: `apps/server/core/AIServer/tests/A2AGameServiceTest.cpp`

- [ ] **Step 1: Implement room lifecycle methods**

Add `CreateRoom`, `ListRooms`, `GetRoomState`, `JoinRoom`, `AddAgent`, `StartRoom`, `SubmitAction`, and `TickRoom`.

- [ ] **Step 2: Implement public state filtering**

`GetRoomState(uid, room_id)` must return secret role data only for the requesting human participant and must not expose other participants' `private_state_json`.

- [ ] **Step 3: Implement explicit agent tick**

`TickRoom` finds `NextPendingActor`. If that actor is an agent, call `A2AGameAgentRunner`; if it is a human, return state unchanged with a message that the room is waiting for human action.

- [ ] **Step 4: Implement agent fallback action**

If the agent response is not valid JSON or chooses an unavailable action, append an `agent_error` event and apply `skip` if `skip` is available.

- [ ] **Step 5: Add service tests**

Use an in-memory fake repo where possible. Cover create room, add agent, start game, submit human speak action, and tick agent fallback.

---

### Task 6: Extend Protobuf And AIServer RPC

**Files:**

- Modify: `apps/server/core/common/proto/ai_message.proto`
- Modify: `apps/server/core/AIServer/AIServiceCore.h`
- Modify: `apps/server/core/AIServer/AIServiceCore.cpp`
- Modify: `apps/server/core/AIServer/AIServiceImpl.h`
- Modify: `apps/server/core/AIServer/AIServiceImpl.cpp`
- Modify: `apps/server/core/AIServer/CMakeLists.txt`

- [ ] **Step 1: Add protobuf messages**

Add a generic game request/response pair to avoid a large proto surface in the first version:

```proto
message AIGameReq {
    int32 uid = 1;
    string room_id = 2;
    string operation = 3;
    string payload_json = 4;
}

message AIGameRsp {
    int32 code = 1;
    string message = 2;
    string state_json = 3;
}
```

- [ ] **Step 2: Add service methods**

Add:

```proto
rpc GameRoomCreate(AIGameReq) returns (AIGameRsp) {}
rpc GameRoomList(AIGameReq) returns (AIGameRsp) {}
rpc GameRoomState(AIGameReq) returns (AIGameRsp) {}
rpc GameRoomJoin(AIGameReq) returns (AIGameRsp) {}
rpc GameAgentAdd(AIGameReq) returns (AIGameRsp) {}
rpc GameStart(AIGameReq) returns (AIGameRsp) {}
rpc GameAction(AIGameReq) returns (AIGameRsp) {}
rpc GameTick(AIGameReq) returns (AIGameRsp) {}
```

- [ ] **Step 3: Delegate in `AIServiceCore`**

Add `std::unique_ptr<A2AGameService> _game_service;` and handler methods that pass request fields into the service.

- [ ] **Step 4: Expose in `AIServiceImpl`**

Add gRPC method implementations with spans named `AIService.GameRoomCreate`, `AIService.GameRoomList`, and matching names for the remaining methods.

- [ ] **Step 5: Update CMake**

Add new game `.cpp` files to the AIServer target and add new tests to the test target.

---

### Task 7: Add GateServer HTTP Routes

**Files:**

- Modify: `apps/server/core/GateServer/AIServiceClient.h`
- Modify: `apps/server/core/GateServer/AIServiceClient.cpp`
- Modify: `apps/server/core/GateServer/AIRouteModules.cpp`

- [ ] **Step 1: Add client methods**

Add one method per route returning `memochat::json::JsonValue`, following existing `Chat`, `Smart`, and `AgentTask*` patterns.

- [ ] **Step 2: Add route handlers**

Register the eight routes from the design spec. Each handler must parse JSON or query params, pass `uid`, `room_id`, and `payload_json` to AIServer, and write JSON with `WriteJsonResponse`.

- [ ] **Step 3: Add input checks**

Return `{"error":1,"message":"invalid json"}` for parse failures. Return `{"error":1,"message":"room_id is empty"}` for state/action/tick routes that require a room.

- [ ] **Step 4: Add logs and spans**

Use span names like `gate.ai.game.room_create` and log result codes with room id and uid.

---

### Task 8: Add AgentController Game State And HTTP Calls

**Files:**

- Modify: `apps/client/desktop/MemoChat-qml/AgentController.h`
- Modify: `apps/client/desktop/MemoChat-qml/AgentController.cpp`

- [ ] **Step 1: Add QML properties**

Add `gameRooms`, `currentGameRoomId`, `currentGameState`, `gameBusy`, `gameError`, and `gameTimeline` properties with notify signals.

- [ ] **Step 2: Add invokable methods**

Add `createGameRoom`, `loadGameRooms`, `loadGameState`, `joinGameRoom`, `addGameAgent`, `startGameRoom`, `submitGameAction`, and `tickGameRoom`.

- [ ] **Step 3: Reuse model selection**

When adding an agent from the UI, default `model_type` and `model_name` to `_current_model_backend` and `_current_model_name`.

- [ ] **Step 4: Parse responses**

For successful game responses, parse `state_json` into a `QVariantMap`/`QVariantList` shape consumed by QML. For errors, set `gameError` and emit `gameStateChanged`.

- [ ] **Step 5: Add polling timer only when active**

Use a `QTimer` owned by `AgentController` to call `loadGameState(_current_game_room_id)` every 3000 ms while a game room is open and not ended.

---

### Task 9: Add QML Game Pane

**Files:**

- Modify: `apps/client/desktop/MemoChat-qml/qml/agent/AgentPane.qml`
- Create: `apps/client/desktop/MemoChat-qml/qml/agent/game/A2AGamePane.qml`
- Create: `apps/client/desktop/MemoChat-qml/qml/agent/game/GameRoomList.qml`
- Create: `apps/client/desktop/MemoChat-qml/qml/agent/game/GameRoomHeader.qml`
- Create: `apps/client/desktop/MemoChat-qml/qml/agent/game/GameParticipantList.qml`
- Create: `apps/client/desktop/MemoChat-qml/qml/agent/game/GameTimeline.qml`
- Create: `apps/client/desktop/MemoChat-qml/qml/agent/game/GameActionBar.qml`
- Create: `apps/client/desktop/MemoChat-qml/qml/agent/game/CreateGameRoomDialog.qml`
- Modify: `apps/client/desktop/MemoChat-qml/qml.qrc`

- [ ] **Step 1: Add the `游戏` header action**

In `AgentPane.qml`, add a compact button near existing actions and show/hide `A2AGamePane` without replacing the normal AI conversation model.

- [ ] **Step 2: Build room list**

Show title, ruleset, status, phase, participant count, and updated time. Provide create and refresh controls.

- [ ] **Step 3: Build active room view**

Show header, participants, timeline, and action bar. Keep all labels bounded with elide/wrap behavior matching existing QML style.

- [ ] **Step 4: Build create dialog**

Fields: title, ruleset selector with `werewolf.basic`, max players, starting agent count, and persona style.

- [ ] **Step 5: Build action bar**

Show actions returned by `available_actions`. Implement speak text input, vote target selector, skip, start, and tick buttons.

- [ ] **Step 6: Register resources**

Add all new QML files to `qml.qrc` so packaged builds can load them.

---

### Task 10: Build And Runtime Smoke

**Files:**

- No source changes unless verification reveals defects.

- [ ] **Step 1: Configure full build**

Run:

```powershell
cmake --preset msvc2022-full
```

Expected: configure completes and protobuf files regenerate.

- [ ] **Step 2: Build full stack**

Run:

```powershell
cmake --build --preset msvc2022-full
```

Expected: `AIServer.exe`, `GateServer.exe`, and `MemoChatQml.exe` build in `build\bin\Release`.

- [ ] **Step 3: Run focused tests**

Run:

```powershell
ctest --preset msvc2022-full -R "A2AGame|Werewolf|AIServiceJsonMapper" --output-on-failure
```

Expected: new game tests and existing AI JSON mapper tests pass.

- [ ] **Step 4: Check Docker dependencies**

Run:

```powershell
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
docker exec memochat-postgres psql -U memochat -d memo_pg -c "select 1;"
```

Expected: PostgreSQL responds with one row containing `1`; no stable published ports are changed.

- [ ] **Step 5: Deploy and smoke GateServer endpoints**

Run:

```powershell
tools\scripts\status\deploy_services.bat
tools\scripts\status\start-all-services.bat
```

Then probe create/list/state/action/tick endpoints with `Invoke-WebRequest`. Expected: a room can be created, listed, started, ticked, and read back through GateServer.

- [ ] **Step 6: Manual UI smoke**

Open MemoChatQml, enter AI assistant, open `游戏`, create a Werewolf room with at least two agents, start it, tick one agent turn, submit a human speak or skip action, and confirm the timeline updates without overlapping text.

---

## Delivery Milestones

1. **Backend skeleton:** schema, domain structs, ruleset tests, and service tests pass.
2. **API skeleton:** GateServer routes can create/list/state/start/action/tick through AIServer.
3. **UI skeleton:** AI chat pane can create and operate a basic room.
4. **Playable MVP:** one human plus multiple agents can complete a small `werewolf.basic` game loop.

## Risk Controls

- Keep first version explicit-tick based so agent loops are deterministic and testable.
- Keep state in JSONB plus events so schema changes do not block early ruleset iteration.
- Keep rules compiled in C++ so user-created game content cannot execute code.
- Do not change Docker port mappings.
- Do not route through ChatServer until HTTP polling proves the gameplay model.

## Self-Review

- Spec coverage: room lifecycle, agents, human joins, ruleset contract, QML surface, persistence, and verification are each mapped to tasks.
- Placeholder scan: no task relies on unspecified file names or undefined route names.
- Type consistency: `AIGameReq`, `AIGameRsp`, `A2AGameService`, `A2AGameRuleEngine`, and QML property names are used consistently across tasks.
