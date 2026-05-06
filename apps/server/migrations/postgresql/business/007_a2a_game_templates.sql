CREATE TABLE IF NOT EXISTS ai_game_template (
    template_id VARCHAR(64) PRIMARY KEY,
    uid INTEGER NOT NULL,
    title TEXT NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT '',
    ruleset_id VARCHAR(64) NOT NULL DEFAULT 'werewolf.basic',
    max_players INTEGER NOT NULL DEFAULT 12,
    agents_json JSONB NOT NULL DEFAULT '[]'::jsonb,
    config_json JSONB NOT NULL DEFAULT '{}'::jsonb,
    metadata_json JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at BIGINT NOT NULL,
    updated_at BIGINT NOT NULL,
    deleted_at BIGINT DEFAULT NULL
);

CREATE INDEX IF NOT EXISTS idx_ai_game_template_uid_updated
    ON ai_game_template (uid, updated_at DESC);

CREATE INDEX IF NOT EXISTS idx_ai_game_template_ruleset_updated
    ON ai_game_template (ruleset_id, updated_at DESC);
