CREATE TABLE IF NOT EXISTS ai_game_room_state (
    room_id VARCHAR(64) PRIMARY KEY,
    owner_uid INTEGER NOT NULL,
    ruleset_id VARCHAR(64) NOT NULL,
    status VARCHAR(32) NOT NULL,
    phase VARCHAR(32) NOT NULL,
    title TEXT NOT NULL DEFAULT '',
    snapshot_json JSONB NOT NULL,
    created_at BIGINT NOT NULL,
    updated_at BIGINT NOT NULL,
    deleted_at BIGINT DEFAULT NULL
);

CREATE INDEX IF NOT EXISTS idx_ai_game_room_state_owner_updated
    ON ai_game_room_state (owner_uid, updated_at DESC);

CREATE INDEX IF NOT EXISTS idx_ai_game_room_state_ruleset_updated
    ON ai_game_room_state (ruleset_id, updated_at DESC);

CREATE INDEX IF NOT EXISTS idx_ai_game_room_state_status_updated
    ON ai_game_room_state (status, updated_at DESC);
