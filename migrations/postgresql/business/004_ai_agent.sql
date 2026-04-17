-- AI 会话表：存储用户与 AI 的对话会话
CREATE TABLE IF NOT EXISTS ai_session (
    id BIGSERIAL PRIMARY KEY,
    session_id VARCHAR(64) NOT NULL,
    uid INTEGER NOT NULL,
    title VARCHAR(256) NOT NULL DEFAULT '',
    model_type VARCHAR(32) NOT NULL DEFAULT 'ollama',
    model_name VARCHAR(64) NOT NULL,
    created_at BIGINT NOT NULL,
    updated_at BIGINT NOT NULL,
    deleted_at BIGINT DEFAULT NULL,
    CONSTRAINT uq_ai_session_session_id UNIQUE (session_id),
    CONSTRAINT uq_ai_session_uid_session UNIQUE (uid, session_id)
);

-- 索引：按用户查询会话列表
CREATE INDEX IF NOT EXISTS idx_ai_session_uid_updated
    ON ai_session (uid, updated_at DESC);

-- AI 消息表：存储会话中的每条消息
CREATE TABLE IF NOT EXISTS ai_message (
    id BIGSERIAL PRIMARY KEY,
    msg_id VARCHAR(64) NOT NULL,
    session_id VARCHAR(64) NOT NULL,
    role VARCHAR(16) NOT NULL,
    content TEXT NOT NULL DEFAULT '',
    tokens_used BIGINT NOT NULL DEFAULT 0,
    model_name VARCHAR(64) NOT NULL DEFAULT '',
    created_at BIGINT NOT NULL,
    deleted_at BIGINT DEFAULT NULL,
    CONSTRAINT uq_ai_message_msg_id UNIQUE (msg_id)
);

-- 索引：按会话查询消息
CREATE INDEX IF NOT EXISTS idx_ai_message_session_created
    ON ai_message (session_id, created_at DESC);

-- AI 智能功能使用记录：用于审计和计费
CREATE TABLE IF NOT EXISTS ai_smart_log (
    log_id BIGSERIAL PRIMARY KEY,
    uid INTEGER NOT NULL,
    feature_type VARCHAR(32) NOT NULL,
    input_tokens INTEGER DEFAULT NULL,
    output_tokens INTEGER DEFAULT NULL,
    model_name VARCHAR(64) DEFAULT NULL,
    created_at BIGINT NOT NULL
);

-- 索引：按用户和时间查询使用记录
CREATE INDEX IF NOT EXISTS idx_ai_smart_log_uid_created
    ON ai_smart_log (uid, created_at DESC);

-- AI 知识库元数据表：记录 Qdrant collection 映射关系
CREATE TABLE IF NOT EXISTS ai_knowledge_base (
    id BIGSERIAL PRIMARY KEY,
    kb_id VARCHAR(64) NOT NULL,
    uid INTEGER NOT NULL,
    name VARCHAR(256) NOT NULL,
    file_type VARCHAR(16) NOT NULL,
    chunk_count INTEGER NOT NULL DEFAULT 0,
    qdrant_collection VARCHAR(128) NOT NULL,
    status VARCHAR(16) NOT NULL DEFAULT 'indexing',
    created_at BIGINT NOT NULL,
    updated_at BIGINT NOT NULL,
    deleted_at BIGINT DEFAULT NULL,
    CONSTRAINT uq_ai_knowledge_base_kb_id UNIQUE (kb_id)
);

-- 索引：按用户查询知识库列表
CREATE INDEX IF NOT EXISTS idx_ai_knowledge_base_uid_created
    ON ai_knowledge_base (uid, created_at DESC);

-- AI 情景记忆表：跨会话重要交互（用于三层记忆系统）
CREATE TABLE IF NOT EXISTS ai_episodic_memory (
    id BIGSERIAL PRIMARY KEY,
    uid INTEGER NOT NULL,
    session_id VARCHAR(64) DEFAULT NULL,
    summary TEXT NOT NULL,
    entities JSONB NOT NULL DEFAULT '{}',
    importance FLOAT NOT NULL DEFAULT 0.5,
    created_at BIGINT NOT NULL
);

-- 索引：按用户和重要性查询
CREATE INDEX IF NOT EXISTS idx_ai_episodic_memory_uid_importance
    ON ai_episodic_memory (uid, importance DESC, created_at DESC);

-- AI 语义记忆表：结构化用户偏好（用于三层记忆系统）
CREATE TABLE IF NOT EXISTS ai_semantic_memory (
    id BIGSERIAL PRIMARY KEY,
    uid INTEGER NOT NULL,
    preferences JSONB NOT NULL DEFAULT '{}',
    updated_at BIGINT NOT NULL,
    CONSTRAINT uq_ai_semantic_memory_uid UNIQUE (uid)
);
