CREATE TABLE IF NOT EXISTS chat_event_outbox (
    id BIGSERIAL PRIMARY KEY,
    event_id VARCHAR(64) NOT NULL,
    topic VARCHAR(128) NOT NULL,
    partition_key VARCHAR(128) NOT NULL,
    payload_json JSONB NOT NULL,
    status SMALLINT NOT NULL DEFAULT 0,
    retry_count INT NOT NULL DEFAULT 0,
    next_retry_at BIGINT NOT NULL DEFAULT 0,
    created_at BIGINT NOT NULL,
    published_at BIGINT NULL,
    last_error TEXT NOT NULL DEFAULT '',
    CONSTRAINT uq_chat_event_outbox_event_id UNIQUE (event_id)
);

CREATE INDEX IF NOT EXISTS idx_chat_event_outbox_status_retry
    ON chat_event_outbox (status, next_retry_at, id);

CREATE INDEX IF NOT EXISTS idx_chat_event_outbox_topic_status
    ON chat_event_outbox (topic, status, id);

CREATE UNIQUE INDEX IF NOT EXISTS uk_chat_private_msg_msg_id
    ON chat_private_msg (msg_id);

CREATE UNIQUE INDEX IF NOT EXISTS uk_chat_group_msg_group_msg_id
    ON chat_group_msg (group_id, msg_id);
