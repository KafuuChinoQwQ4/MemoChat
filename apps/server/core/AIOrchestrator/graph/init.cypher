// 初始化 Neo4j 图数据模型
// 节点类型定义

// 1. User 节点 — 用户
CREATE CONSTRAINT user_uid IF NOT EXISTS
FOR (u:User) REQUIRE u.uid IS UNIQUE;

// 2. Entity 节点 — 实体（从消息中抽取）
CREATE CONSTRAINT entity_name_type IF NOT EXISTS
FOR (e:Entity) REQUIRE (e.name, e.type) IS UNIQUE;

// 3. Group 节点 — 群组
CREATE CONSTRAINT group_gid IF NOT EXISTS
FOR (g:Group) REQUIRE g.group_id IS UNIQUE;

// 4. Topic 节点 — 话题
CREATE CONSTRAINT topic_name IF NOT EXISTS
FOR (t:Topic) REQUIRE t.name IS UNIQUE;

// 5. Message 节点 — 消息
CREATE CONSTRAINT message_mid IF NOT EXISTS
FOR (m:Message) REQUIRE m.mid IS UNIQUE;

// 创建索引
CREATE INDEX user_interests IF NOT EXISTS FOR (u:User) ON (u.interests);
CREATE INDEX user_name IF NOT EXISTS FOR (u:User) ON (u.name);
CREATE INDEX group_name IF NOT EXISTS FOR (g:Group) ON (g.name);
CREATE INDEX topic_name_idx IF NOT EXISTS FOR (t:Topic) ON (t.name);
CREATE INDEX entity_type IF NOT EXISTS FOR (e:Entity) ON (e.type);

// 关系类型说明
// KNOWS — 用户之间互为好友
// MESSAGED — 用户之间有消息往来（带 count 属性表示消息数量）
// IN_GROUP — 用户在某个群组中
// SENT_BY — 消息由某个用户发送
// ABOUT — 消息涉及某个话题
// INTERESTED_IN — 用户对某个话题感兴趣
// HAS_TOPIC — 群组包含某个话题
// MENTIONED — 实体被某条消息提及

RETURN "Graph schema initialized successfully" AS status;