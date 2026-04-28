CREATE TABLE IF NOT EXISTS ai_agent_run_trace (
    id BIGSERIAL PRIMARY KEY,
    trace_id VARCHAR(64) NOT NULL,
    uid INTEGER NOT NULL,
    session_id VARCHAR(64) NOT NULL DEFAULT '',
    skill_name VARCHAR(64) NOT NULL DEFAULT '',
    request_summary TEXT NOT NULL DEFAULT '',
    response_content TEXT NOT NULL DEFAULT '',
    plan_json JSONB NOT NULL DEFAULT '[]'::jsonb,
    observations_json JSONB NOT NULL DEFAULT '[]'::jsonb,
    model_name VARCHAR(128) NOT NULL DEFAULT '',
    status VARCHAR(32) NOT NULL DEFAULT 'running',
    feedback_summary TEXT NOT NULL DEFAULT '',
    started_at BIGINT NOT NULL,
    finished_at BIGINT DEFAULT NULL,
    updated_at BIGINT NOT NULL,
    deleted_at BIGINT DEFAULT NULL,
    CONSTRAINT uq_ai_agent_run_trace_trace_id UNIQUE (trace_id)
);

CREATE INDEX IF NOT EXISTS idx_ai_agent_run_trace_uid_started
    ON ai_agent_run_trace (uid, started_at DESC);

CREATE INDEX IF NOT EXISTS idx_ai_agent_run_trace_session_started
    ON ai_agent_run_trace (session_id, started_at DESC);

CREATE TABLE IF NOT EXISTS ai_agent_run_step (
    id BIGSERIAL PRIMARY KEY,
    trace_id VARCHAR(64) NOT NULL,
    step_index INTEGER NOT NULL,
    layer VARCHAR(32) NOT NULL,
    step_name VARCHAR(64) NOT NULL,
    status VARCHAR(32) NOT NULL DEFAULT 'ok',
    summary TEXT NOT NULL DEFAULT '',
    detail TEXT NOT NULL DEFAULT '',
    metadata_json JSONB NOT NULL DEFAULT '{}'::jsonb,
    duration_ms BIGINT NOT NULL DEFAULT 0,
    created_at BIGINT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_ai_agent_run_step_trace_step
    ON ai_agent_run_step (trace_id, step_index);

CREATE TABLE IF NOT EXISTS ai_agent_feedback (
    id BIGSERIAL PRIMARY KEY,
    trace_id VARCHAR(64) NOT NULL,
    uid INTEGER NOT NULL,
    rating INTEGER NOT NULL,
    feedback_type VARCHAR(64) NOT NULL DEFAULT 'user_rating',
    comment TEXT NOT NULL DEFAULT '',
    payload_json JSONB NOT NULL DEFAULT '{}'::jsonb,
    created_at BIGINT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_ai_agent_feedback_trace_created
    ON ai_agent_feedback (trace_id, created_at DESC);
