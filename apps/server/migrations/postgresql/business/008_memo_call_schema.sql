-- gateserver split Phase 2 (slice 2a): memo_call database schema.
-- Loaded into the memo_call database. Owns call session records (LiveKit rooms +
-- Redis call:* keys are runtime state, not here). Mirrors memo.chat_call_session.

CREATE SCHEMA IF NOT EXISTS memo;

CREATE TABLE IF NOT EXISTS memo.chat_call_session (
    call_id character varying(64) NOT NULL PRIMARY KEY,
    room_name character varying(128) NOT NULL,
    call_type character varying(16) NOT NULL,
    caller_uid integer NOT NULL,
    callee_uid integer NOT NULL,
    state character varying(32) NOT NULL,
    started_at_ms bigint NOT NULL,
    accepted_at_ms bigint DEFAULT 0 NOT NULL,
    ended_at_ms bigint DEFAULT 0 NOT NULL,
    expires_at_ms bigint DEFAULT 0 NOT NULL,
    duration_sec integer DEFAULT 0 NOT NULL,
    reason character varying(64) DEFAULT ''::character varying NOT NULL,
    trace_id character varying(64) DEFAULT ''::character varying NOT NULL,
    updated_at_ms bigint DEFAULT 0 NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_chat_call_session_callee
    ON memo.chat_call_session USING btree (callee_uid, started_at_ms);
CREATE INDEX IF NOT EXISTS idx_chat_call_session_caller
    ON memo.chat_call_session USING btree (caller_uid, started_at_ms);
CREATE INDEX IF NOT EXISTS idx_chat_call_session_state
    ON memo.chat_call_session USING btree (state, updated_at_ms);

GRANT USAGE ON SCHEMA memo TO memo_call_app;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA memo TO memo_call_app;
GRANT USAGE, SELECT, UPDATE ON ALL SEQUENCES IN SCHEMA memo TO memo_call_app;
ALTER DEFAULT PRIVILEGES IN SCHEMA memo GRANT SELECT, INSERT, UPDATE, DELETE ON TABLES TO memo_call_app;
ALTER DEFAULT PRIVILEGES IN SCHEMA memo GRANT USAGE, SELECT, UPDATE ON SEQUENCES TO memo_call_app;
