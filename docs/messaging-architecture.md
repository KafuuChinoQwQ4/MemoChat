# MemoChat 消息架构文档

> 文档版本：2026-04-10
> 项目路径：D:\MemoChat-Qml-Drogon

---

## 一、消息架构总览

### 1.1 双总线协同设计理念

MemoChat 采用**双消息总线 + Redis** 的三层消息架构，每层承担不同的职责：

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              QML 客户端                                      │
│           TCP/QUIC 长连接 ──► ChatServer (本地消费)                          │
│           HTTP Polling ──► GateServer (拉取离线消息)                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          ChatServer ×4 节点集群                              │
│                                                                              │
│   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                │
│   │  Kafka       │    │  RabbitMQ    │    │    Redis     │                │
│   │  事件总线     │    │  任务总线    │    │  在线状态     │                │
│   │  (事实流)    │    │  (任务流)    │    │  (会话缓存)   │                │
│   └──────────────┘    └──────────────┘    └──────────────┘                │
│                                                                              │
│   职责划分：                                                                 │
│   - Kafka: 聊天消息事实流、对话同步、关系状态变更、审计事件                    │
│   - RabbitMQ: 消息投递重试、离线通知、邮件投递、缓存失效                      │
│   - Redis: 在线状态、会话缓存、分布式锁、Pub/Sub 实时通知                     │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
                    ┌─────────────────┼─────────────────┐
                    │                 │                 │
                    ▼                 ▼                 ▼
            ┌────────────┐   ┌────────────┐   ┌────────────┐
            │  PostgreSQL │   │  MongoDB   │   │  MinIO/S3  │
            │ (元数据)    │   │ (消息历史)  │   │ (媒体文件)  │
            └────────────┘   └────────────┘   └────────────┘
```

### 1.2 三层消息系统职责矩阵

| 层级 | 组件 | 数据类型 | 时效性 | 持久化 | 消费模式 | 典型场景 |
|------|------|----------|--------|--------|----------|----------|
| **事实层** | Kafka | 聊天消息、关系变更、审计事件 | 毫秒级 | 7天+ | Consumer Group 并行消费 | 私聊/群聊消息分发 |
| **任务层** | RabbitMQ | 投递重试、离线通知、邮件任务 | 秒级 | 可选 | 独占队列 + 手动 ACK | 消息重试、验证码邮件 |
| **缓存层** | Redis | 在线状态、会话信息、实时通知 | 微秒级 | 内存 | 发布订阅、Key-Value | 好友上线通知、Session 管理 |
| **存储层** | PostgreSQL | 消息索引、关系元数据、会话列表 | - | 持久 | 同步写入 | 消息索引、好友关系 |
| **存储层** | MongoDB | 消息完整内容 | - | 持久 | 同步写入 | 聊天历史、拉取消息 |

### 1.3 为什么选择双总线架构

**单用 Kafka 的问题**：
- Kafka 不擅长处理"需要可靠投递 + 重试 + 死信队列"的**任务型消息**
- Kafka 消费者组重平衡（rebalance）延迟高，不适合短生命周期任务
- Kafka 不支持"独占消费"模式，多个消费者处理同一任务是危险的

**单用 RabbitMQ 的问题**：
- RabbitMQ 在高吞吐（10万+/秒）消息分发场景下性能不如 Kafka
- RabbitMQ 不支持消息回溯（offset replay），无法重放历史消息
- RabbitMQ 的 queue 是临时性的，重启后消息丢失（除非开启持久化）

**MemoChat 的选择**：
- **Kafka**：处理聊天消息的事实流（高吞吐、需要持久化和回溯）
- **RabbitMQ**：处理投递任务、离线通知、邮件等需要可靠投递的任务
- **Redis**：处理实时状态和 Pub/Sub 通知（微秒级延迟）

---

## 二、Kafka 事件总线架构

### 2.1 Topic 设计与分区策略

#### 2.1.1 Topic 一览表

| Topic 名称 | 分区数 | 副本数 | 保留策略 | 生产者 | 消费者 | 用途 |
|------------|--------|--------|----------|--------|--------|------|
| `chat.private.v1` | 8 | 3 | 7 天 | ChatServer | ChatServer ×4 | 私聊消息事实流 |
| `chat.group.v1` | 8 | 3 | 7 天 | ChatServer | ChatServer ×4 | 群聊消息事实流 |
| `dialog.sync.v1` | 8 | 3 | 3 天 | ChatServer | ChatServer ×4 | 对话列表同步 |
| `relation.state.v1` | 4 | 3 | 7 天 | ChatServer | ChatServer ×4 | 好友关系状态变更 |
| `user.profile.changed.v1` | 4 | 3 | 3 天 | GateServer | GateServer ×N | 用户资料变更广播 |
| `audit.login.v1` | 2 | 3 | 30 天 | GateServer | MemoOps | 登录审计事件 |

#### 2.1.2 分区键（Partition Key）设计原则

分区键决定了消息发送到哪个 Partition，直接影响消息的顺序保证范围：

```cpp
// ChatServer/KafkaAsyncEventBus.cpp

// 私聊消息: 按 (min(uid1, uid2) % partition_count) 分区
// 保证同一对话的所有消息落在同一 Partition，确保顺序
std::string MakePrivateMessageKey(int64_t uid1, int64_t uid2) {
    int64_t min_uid = std::min(uid1, uid2);
    int64_t max_uid = std::max(uid1, uid2);
    return std::to_string(min_uid) + "_" + std::to_string(max_uid);
}

// 群聊消息: 按 group_id % partition_count 分区
// 保证同一群的所有消息落在同一 Partition
std::string MakeGroupMessageKey(int64_t group_id) {
    return "group_" + std::to_string(group_id);
}

// 对话同步: 按 uid % partition_count 分区
// 保证同一用户的所有对话同步消息落在同一 Partition
std::string MakeDialogSyncKey(int64_t uid) {
    return "dialog_" + std::to_string(uid);
}
```

> **面试点**：为什么私聊消息用 `(min(uid, uid2) % N)` 而不是 `uid % N`？
> - 如果按发送方 uid 分区，发送方不在线时消息会丢失（无法发送到对应 Partition）
> - 用两个 uid 的最小值作为分区键，无论谁发消息，同一会话的所有消息都落在同一 Partition
> - 而且 `min(a,b) == min(b,a)`，双向消息天然聚合在一起

#### 2.1.3 分区数与消费者数量的关系

```
                    Partition 分配示意图
                    
Topic: chat.private.v1 (8 Partitions)
                    
Consumer Group: chatserver-cluster (4 ChatServer 节点)
                    
Partition 0 ──► ChatServer-1  (Assigned: P0, P4)
Partition 1 ──► ChatServer-2  (Assigned: P1, P5)
Partition 2 ──► ChatServer-3  (Assigned: P2, P6)
Partition 3 ──► ChatServer-4  (Assigned: P3, P7)
Partition 4 ──► ChatServer-1
Partition 5 ──► ChatServer-2
Partition 6 ──► ChatServer-3
Partition 7 ──► ChatServer-4
```

**设计原则**：
- 分区数 = 消费者数的整数倍（便于均匀分配）
- 当前 4 个 ChatServer 节点，8 个分区 = 2 倍冗余（每个节点消费 2 个分区）
- 扩缩容时，Kafka 会自动重新分配 Partition（rebalance）

### 2.2 消息格式定义

#### 2.2.1 消息Envelope 结构

所有 Kafka 消息统一使用以下 Envelope 结构：

```json
{
  "event_id": "evt-uuid-123",           // 全局唯一事件 ID (Snowflake)
  "event_type": "private_message",        // 事件类型
  "event_version": 1,                     // 事件版本（用于 Schema 演进）
  "trace_id": "trace-abc123",            // 链路追踪 ID
  "timestamp_ms": 1711180800000,         // 事件产生时间（毫秒）
  "source_service": "chatserver1",       // 产生事件的服务
  "source_instance": "chatserver1-1",    // 服务实例
  "payload": {
    // 具体业务数据
  }
}
```

#### 2.2.2 私聊消息 Payload

```json
{
  "event_type": "private_message",
  "payload": {
    "msg_id": "msg-uuid-789",
    "from_uid": 12345,
    "to_uid": 67890,
    "content": "Hello!",
    "msg_type": "text",
    "client_time_ms": 1711180800000,
    "server_time_ms": 1711180800001,
    "reply_to_msg_id": null,
    "mentions": [],
    "forward_from": null
  }
}
```

#### 2.3 生产者配置详解

```cpp
// ChatServer/KafkaAsyncEventBus.cpp

// Kafka 生产者配置
KafkaProducerConfig config = {
    .brokers = "127.0.0.1:19092",
    .acks = "all",                        // 等待所有 ISR 确认，最安全
    .enable_idempotence = true,           // 开启幂等性，防止重复发送
    .retries = 3,                         // 重试次数
    .retry_backoff_ms = 100,              // 重试间隔
    .linger_ms = 5,                       // 批量发送延迟（吞吐 vs 延迟权衡）
    .batch_size = 16384,                  // 批量大小 (16KB)
    .compression = "lz4",                 // 压缩算法（lz4 压缩率高且 CPU 开销小）
    .max_in_flight_requests_per_connection = 5,  // 允许最多 5 个请求在飞
    .request_timeout_ms = 30000,          // 请求超时
    .delivery_timeout_ms = 120000         // 投递总超时（含重试）
};
```

> **面试点**：acks=all vs acks=1 的权衡
> - `acks=1`：Leader 写入即返回，不等 Follower 同步。如果 Leader 写入后宕机，Follower 还没同步，消息丢失
> - `acks=all`：所有 ISR（In-Sync Replicas）都确认后才返回。即使 Leader 宕机，所有 ISR 都有数据，不会丢消息
> - MemoChat 选择 `acks=all + enable_idempotence=true`，确保消息不丢不重

#### 2.3.1 幂等性保证（Exactly-Once Semantics）

```cpp
// 开启 idempotence 后，Kafka 自动为每个 Producer 分配 Producer ID (PID)
// 每个 Partition 上，PID + Sequence Number 唯一标识一条消息
// Kafka 会自动去重：同一 PID + Seq 的消息只写入一次

// 幂等性配置的效果：
// - 解决了"生产者重试导致消息重复"的问题
// - 但不能解决"消费者处理失败后重新消费"的问题（需要业务端幂等）
// - MemoChat 的消息 ID (msg_id) 是客户端生成的 UUID，天然幂等
```

### 2.4 消费者配置详解

#### 2.4.1 ChatServer 消费者配置

```cpp
// ChatServer/KafkaAsyncEventBus.cpp

// Kafka 消费者配置
KafkaConsumerConfig config = {
    .brokers = "127.0.0.1:19092",
    .group_id = "chatserver-cluster",     // 消费者组 ID
    .auto_offset_reset = "earliest",       // 无 offset 时从最早开始消费
    .enable_auto_commit = false,           // 手动提交 offset（精确控制）
    .auto_commit_interval_ms = 5000,       // 自动提交间隔（仅 enable_auto_commit=true 时）
    .session_timeout_ms = 30000,           // 消费者 Session 超时
    .max_poll_interval_ms = 300000,       // 最大 poll 间隔（处理慢的消费者）
    .max_poll_records = 500,              // 每次 poll 最多拉取 500 条消息
    .fetch_min_bytes = 1,                 // 最小 fetch 大小（1 即立即返回）
    .fetch_max_wait_ms = 500,             // 最大等待时间
    .isolation_level = "read_uncommitted"  // 可以读到未提交的消息
};
```

#### 2.4.2 消费者线程模型

每个 ChatServer 节点启动多个消费者线程：

```
ChatServer-1 线程模型：

┌─────────────────────────────────────────────────────────────┐
│                     KafkaConsumerManager                     │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │ Consumer-0  │  │ Consumer-1  │  │ Consumer-2  │  ...  │
│  │ (P0)        │  │ (P1)        │  │ (P2)        │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
│         │                │                │                 │
│         ▼                ▼                ▼                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              MessageHandlerPool (线程池)             │   │
│  │  处理消息的业务逻辑（写入 MongoDB、推送 TCP 等）       │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 2.5 Kafka 消费流程详解

#### 2.5.1 私聊消息消费流程

```
用户 A (在 ChatServer-1) 发送消息给用户 B (在 ChatServer-2)：

Step 1: ChatServer-1 接收消息
        A ──TCP/QUIC──► ChatServer-1
                              │
                              ▼
                         检查 B 在线状态
                         (Redis: online:{B})
                              │
                              │ B 在线，获取 B 所在的 ChatServer
                              ▼
Step 2: 写入存储
        ┌─────────────────────────────────────┐
        │ PostgreSQL: INSERT chat_private_msg │
        │ MongoDB: INSERT private_messages    │
        │ Redis: 更新对话列表缓存               │
        └─────────────────────────────────────┘
                              │
                              ▼
Step 3: 发布到 Kafka (chat.private.v1)
        Partition Key = min(A,B) + "_" + max(A,B)
        Envelope: {
          "event_type": "private_message",
          "payload": { msg_id, from_uid, to_uid, content, ... }
        }
                              │
                              ▼
Step 4: Kafka Broker 持久化 (Leader ──► Followers 同步)
        acks=all 确保所有 ISR 都持久化成功

                              │
                              ▼
Step 5: ChatServer-2 消费消息 (同一 Partition)
        消费者线程从 Kafka 拉取消息
                              │
                              ▼
Step 6: 检查 B 在线并推送
        ┌─────────────────────────────────────────┐
        │ Redis: online:{B} ──► ChatServer-2?    │
        │ 是 ──► 通过 TCP/QUIC 推送 ID_NOTIFY_TEXT_CHAT_MSG_REQ │
        │ 否 ──► 发布到 RabbitMQ (chat.notify.offline) 离线通知 │
        └─────────────────────────────────────────┘
                              │
                              ▼
Step 7: 手动提交 offset
        commit_offset(P0, offset=12345)
```

### 2.6 Consumer Group 与 Rebalance

#### 2.6.1 Rebalance 触发条件

Rebalance（消费者组再平衡）会在以下情况触发：

```
1. 新消费者加入组
   ChatServer-1 ──► ChatServer-1, ChatServer-2 ──► ChatServer-1, ChatServer-2, ChatServer-3
   (2 个 Partition 分配)    (3 个 Partition 分配)        (4 个 Partition 分配)
   
2. 消费者离开组
   ChatServer-3 宕机 ──► P2, P3 重新分配给 ChatServer-1, ChatServer-2

3. Partition 数量变更
   管理员增加 Topic 分区数 ──► 重新分配所有 Partition

4. 消费者 Session 超时
   消费者处理消息超过 session_timeout_ms ──► 被踢出组 ──► Rebalance
```

#### 2.6.2 Rebalance 期间的可用性问题

```cpp
// Rebalance 期间：
// 1. 所有消费者停止消费（Stop The World）
// 2. Consumer Group Leader 重新计算 Partition 分配
// 3. 将分配方案通过 Group Coordinator 通知所有消费者
// 4. 消费者重新开始消费

// MemoChat 的优化策略：
// 1. 设置 session_timeout_ms = 30000（足够长，避免频繁 Rebalance）
// 2. 设置 max_poll_records = 500（单次处理量适中，避免处理超时）
// 3. 使用独立的消费者线程，不阻塞主业务线程
// 4. 在 Rebalance 发生前，尝试提交已处理的 offset（减少重复消费）
```

---

## 三、RabbitMQ 任务总线架构

### 3.1 为什么需要 RabbitMQ

Kafka 的强项是**高吞吐的消息分发**，但不适合以下场景：

1. **可靠任务投递**：验证码邮件必须送达，失败要重试，不能丢失
2. **独占消费**：多个消费者不能同时处理同一个任务
3. **丰富的路由**：根据 routing key 灵活路由到不同队列
4. **死信队列**：消息处理失败后进入 DLQ，不阻塞正常队列
5. **优先级队列**：紧急任务可以优先处理

### 3.2 Exchange 与 Queue 拓扑

```
                    Exchange 拓扑

┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                              │
│  ┌──────────────────────┐                                                   │
│  │   memochat.direct    │  (Direct Exchange, durable)                        │
│  └──────────────────────┘                                                   │
│          │  │  │  │  │                                                      │
│    chat.notify  chat.retry  relation.notify  verify.email  gate.cache        │
│          │     │     │        │            │           │                    │
│          ▼     ▼     ▼        ▼            ▼           ▼                    │
│   ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│   │ chat_    │  │ chat_    │  │ relation_│  │ verify_  │  │ gate_    │    │
│   │ notify   │  │ retry    │  │ notify   │  │ email    │  │ cache    │    │
│   │ _queue   │  │ _queue   │  │ _queue   │  │ _queue   │  │ _queue   │    │
│   └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘    │
│        │              │              │              │              │          │
│        ▼              ▼              ▼              ▼              ▼          │
│   (TCP/QUIC 推送)  (重试处理)   (好友通知)    (邮件发送)     (缓存失效)      │
│                                                                              │
│  ┌──────────────────────┐                                                   │
│  │   memochat.dlx       │  (Dead Letter Exchange)                           │
│  └──────────────────────┘                                                   │
│          │                                                                  │
│      (重试 3 次失败)                                                         │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────┐                                                          │
│   │ memochat.dlq │  (Dead Letter Queue, 人工处理)                            │
│   └──────────────┘                                                          │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.3 Queue 配置详解

| Queue 名称 | Durable | 持久化消息 | Consumer | prefetch_count | x-max-priority | 用途 |
|------------|---------|------------|----------|----------------|----------------|------|
| `chat_notify_queue` | 是 | 是 | ChatServer | 10 | 5 | 离线消息通知推送 |
| `chat_retry_queue` | 是 | 是 | ChatServer | 1 | 10 | 消息投递失败重试 |
| `relation_notify_queue` | 是 | 是 | ChatServer | 5 | - | 好友关系变更通知 |
| `verify_email_queue` | 是 | 是 | VarifyServer | 1 | - | 验证码邮件发送 |
| `gate_cache_queue` | 是 | 是 | GateServer | 10 | - | 缓存失效任务 |
| `status_presence_queue` | 是 | 是 | StatusServer | 5 | - | 在线状态修复 |
| `memochat_dlq` | 是 | 是 | 人工处理 | - | - | 死信队列 |

### 3.4 消息投递可靠性保障

#### 3.4.1 手动 ACK 模式

```cpp
// ChatServer/RabbitMqTaskBus.cpp

// 消费消息时使用手动 ACK
void ConsumeMessage(const std::string& queue_name) {
    auto channel = GetChannel();
    auto delivery_tag = envelope.get_delivery_tag();
    
    try {
        // 1. 处理消息业务逻辑
        ProcessMessage(envelope);
        
        // 2. 处理成功，手动 ACK
        channel.basic_ack(delivery_tag, false);  // false=不批量确认
        
    } catch (const RetryableException& e) {
        // 3. 可重试异常，NACK 并重新入队（等待重试）
        // 设置 x-delay 头控制重试延迟
        channel.basic_nack(delivery_tag, false, true);  // requeue=true
        
    } catch (const NonRetryableException& e) {
        // 4. 不可重试异常，NACK 不重新入队（进入 DLQ）
        channel.basic_nack(delivery_tag, false, false);  // requeue=false
        
    } catch (const std::exception& e) {
        // 5. 未知异常，NACK 不重新入队
        channel.basic_nack(delivery_tag, false, false);
    }
}
```

#### 3.4.2 重试策略实现

```cpp
// 使用 RabbitMQ 的 x-delayed-message 插件实现延迟重试

// 首次失败：延迟 1 秒重试
// 第二次失败：延迟 5 秒重试
// 第三次失败：延迟 30 秒重试
// 三次都失败：进入 DLQ

void PublishWithRetry(const std::string& routing_key, 
                      const std::string& message_body,
                      int retry_count) {
    auto channel = GetChannel();
    
    AMQP::Table headers;
    headers["x-retry-count"] = retry_count;
    
    int delay_ms;
    if (retry_count == 0) {
        delay_ms = 1000;      // 1 秒
    } else if (retry_count == 1) {
        delay_ms = 5000;      // 5 秒
    } else if (retry_count == 2) {
        delay_ms = 30000;     // 30 秒
    } else {
        // 超过最大重试次数，进入 DLQ
        channel->publish("memochat.dlx", routing_key, message_body, 
                        durable, priority, headers);
        return;
    }
    
    headers["x-delay"] = delay_ms;
    
    // 发布到延迟交换机
    channel->publish("memochat.delayed", routing_key, message_body,
                    durable, priority, headers);
}
```

#### 3.4.3 消息持久化配置

```cpp
// 生产者发布持久化消息
AMQP::Message message(message_body);
message.setDeliveryMode(AMQP::persistent);  // 持久化消息

// 设置消息优先级（0-10，10 最高）
message.setPriority(5);

// 设置消息过期时间（TTL）
message.setExpiration("60000");  // 60 秒后过期

channel->publish("memochat.direct", routing_key, message);
```

### 3.5 RabbitMQ 与 Kafka 的协同场景

#### 3.5.1 离线消息通知流程

```
用户 B 离线，用户 A 发送消息：

Step 1: ChatServer-1 消费 Kafka 消息 (chat.private.v1)
                              │
                              ▼
Step 2: 检查 B 在线状态
        Redis: online:{B} = false (B 离线)
                              │
                              ▼
Step 3: 发布到 RabbitMQ
        Exchange: memochat.direct
        Routing Key: chat.notify
        Queue: chat_notify_queue
        Message: {
          "to_uid": B,
          "from_uid": A,
          "msg_id": "msg-uuid-789",
          "msg_preview": "Hello!",
          "notify_type": "new_message"
        }
                              │
                              ▼
Step 4: ChatServer-2/3/4 消费消息（随机一个）
        检查 B 是否上线（B 可能刚刚上线）
                              │
                              ▼
Step 5: B 仍然离线，发送推送通知
        - 如果有设备 Token：发送 FCM/APNs 推送
        - 如果没有：只记录离线消息，B 上线后拉取
                              │
                              ▼
Step 6: B 上线，通过 TCP/QUIC 推送离线消息
        客户端主动拉取历史消息填充 UI
```

#### 3.5.2 消息发送失败重试流程

```
消息发送失败（目标用户不在线/网络问题）：

Step 1: ChatServer-2 处理消息失败
        throw RetryableException("User offline, retry later");
                              │
                              ▼
Step 2: NACK 消息，requeue=false
        消息进入 Dead Letter Exchange (memochat.dlx)
                              │
                              ▼
Step 3: DLX 路由到重试队列 (chat_retry_queue)
        设置 x-retry-count 和 x-delay 头
                              │
                              ▼
Step 4: 延迟消息插件等待 x-delay 时间
        (1s / 5s / 30s 递增退避)
                              │
                              ▼
Step 5: 重试队列重新投递到消费者
        消费者再次尝试处理
                              │
                              ▼
Step 6: 仍然失败？回到 Step 2
        或进入 memochat_dlq（人工处理）
```

---

## 四、Redis 实时状态架构

### 4.1 在线状态管理

#### 4.1.1 在线状态 Key 设计

```cpp
// Redis 在线状态管理

// Key Pattern 1: 用户在线状态
// Type: String
// Value: "1" (在线) 或 "0" (离线)
// TTL: 60 秒（心跳续期）
Key: "online:{uid}"
Example: SET online:12345 "1" EX 60

// Key Pattern 2: 用户所在 ChatServer
// Type: String  
// Value: JSON: {"server":"chatserver1","tcp_port":8090,"quic_port":8190,"login_time":1711180800}
// TTL: 60 秒
Key: "chatserver:{uid}"
Example: SET chatserver:12345 '{"server":"chatserver1","tcp_port":8090,"quic_port":8190}' EX 60

// Key Pattern 3: 设备在线状态（支持多设备）
// Type: Hash
// Field: device_id, Value: JSON
// TTL: 每个 field 独立过期
Key: "devices:{uid}"
HSET devices:12345 "device-1" '{"transport":"quic","last_seen":1711180800}'
HSET devices:12345 "device-2" '{"transport":"tcp","last_seen":1711180700}'

// Key Pattern 4: ChatServer 节点在线用户集合
// Type: Set
// Members: uid
// 用途: 快速统计节点在线人数
Key: "chatservers:{server_name}:online_users"
SADD chatservers:chatserver1:online_users 12345 67890 11111
SCARD chatservers:chatserver1:online_users  // 返回在线人数
```

#### 4.1.2 心跳保活机制

```cpp
// ChatServer/UserMgr.cpp

// 客户端每 30 秒发送心跳
// 服务端收到心跳后，更新 TTL

void OnHeartbeat(int64_t uid, const std::string& device_id) {
    auto redis = RedisMgr::Instance();
    
    // 1. 更新在线状态 TTL
    redis->Expire("online:" + std::to_string(uid), 60);
    
    // 2. 更新 ChatServer TTL
    redis->Expire("chatserver:" + std::to_string(uid), 60);
    
    // 3. 更新设备最后活跃时间
    redis->HSet("devices:" + std::to_string(uid), 
                device_id,
                "{\"last_seen\":" + std::to_string(GetCurrentTimestamp()) + "}");
}

// 定时检查：TTL 过期前主动续期
void ScheduleHeartbeatRefresh(int64_t uid) {
    // 每 30 秒执行一次，在 TTL 过期前续期
    // 避免网络抖动导致 TTL 提前过期
    boost::asio::steady_timer timer(io_context, boost::asio::chrono::seconds(30));
    timer.async_wait([this, uid](const boost::system::error_code& ec) {
        if (!ec) {
            RefreshOnlineTTL(uid);
            ScheduleHeartbeatRefresh(uid);  // 循环
        }
    });
}
```

#### 4.1.3 用户上线/下线流程

```cpp
// 用户上线流程
void OnUserLogin(int64_t uid, const std::string& server_name, 
                 int tcp_port, int quic_port) {
    auto redis = RedisMgr::Instance();
    
    // 1. SET NX（仅当 Key 不存在时设置）
    // 如果用户已在其他节点登录（Key 已存在），返回 false
    bool success = redis->SetNX("online:" + uid, "1", 60);
    
    if (!success) {
        // 用户已在其他节点登录，需要踢下线
        // 先获取当前所在节点
        auto current_server = redis->Get("chatserver:" + uid);
        // 通过 gRPC 通知该节点踢用户下线
        NotifyKickUser(uid, "Duplicate login");
    }
    
    // 2. 设置用户所在 ChatServer
    Json::Value server_info;
    server_info["server"] = server_name;
    server_info["tcp_port"] = tcp_port;
    server_info["quic_port"] = quic_port;
    server_info["login_time"] = GetCurrentTimestamp();
    redis->Set("chatserver:" + uid, server_info.toStyledString(), 60);
    
    // 3. 加入节点在线用户集合
    redis->SAdd("chatservers:" + server_name + ":online_users", uid);
    
    // 4. 发布用户上线事件（Pub/Sub）
    redis->Publish("presence:" + uid, 
                  R"({"event":"online","uid":12345,"server":"chatserver1"})");
}

// 用户下线流程
void OnUserLogout(int64_t uid, const std::string& server_name) {
    auto redis = RedisMgr::Instance();
    
    // 1. 删除在线状态
    redis->Del("online:" + uid);
    redis->Del("chatserver:" + uid);
    
    // 2. 从节点在线用户集合移除
    redis->SRem("chatservers:" + server_name + ":online_users", uid);
    
    // 3. 发布用户下线事件
    redis->Publish("presence:" + uid,
                  R"({"event":"offline","uid":12345})");
}
```

### 4.2 好友上线通知（Pub/Sub）

#### 4.2.1 订阅机制

```cpp
// ChatServer/RedisAsyncEventBus.cpp

// 订阅当前节点所有在线用户的好友上线通知
void SubscribeFriendOnlineNotifications(const std::vector<int64_t>& friend_uids) {
    auto redis = RedisMgr::Instance();
    auto subscriber = redis->GetSubscriber();
    
    for (int64_t friend_uid : friend_uids) {
        std::string channel = "presence:" + std::to_string(friend_uid);
        subscriber->subscribe(channel, [this, friend_uid](const std::string& msg) {
            // 解析消息
            Json::Value event;
            Json::CharReaderBuilder builder;
            std::istringstream stream(msg);
            Json::parseFromStream(builder, stream, &event, nullptr);
            
            std::string event_type = event["event"].asString();
            
            if (event_type == "online") {
                // 好友上线，通知当前用户
                NotifyFriendOnline(friend_uid, event["server"].asString());
            } else if (event_type == "offline") {
                // 好友下线
                NotifyFriendOffline(friend_uid);
            }
        });
    }
}

// 通知当前用户的好友上线
void NotifyFriendOnline(int64_t friend_uid, const std::string& server_name) {
    // 1. 查找当前用户是否在线（通过全局 ChatServer 路由）
    // 2. 如果在线，通过 TCP/QUIC 发送好友上线通知
    auto session = FindUserSession(my_uid);
    if (session) {
        Json::Value msg;
        msg["event"] = "friend_online";
        msg["friend_uid"] = friend_uid;
        msg["server"] = server_name;
        session->SendTextMessage(MSG_ID_FRIEND_ONLINE_NOTIFY, msg);
    }
}
```

#### 4.2.2 订阅频道设计

```cpp
// 按用户维度订阅（粒度最细）
// Channel: presence:{uid}
// 订阅后只收到该用户的状态变更

// 按群维度订阅（适合群消息通知）
// Channel: group:{group_id}:msg
// 订阅后收到该群的所有消息通知

// 全局通知频道
// Channel: system:notify
// 系统级广播（维护公告、紧急通知等）
```

### 4.3 会话缓存管理

#### 4.3.1 对话列表缓存

```cpp
// ChatServer/ChatSessionService.cpp

// 对话列表缓存 Key
// Type: Sorted Set
// Score: 最后消息时间戳
// Member: JSON { dialog_type, peer_uid, group_id }
Key: "dialogs:{uid}"
Example: 
  ZADD dialogs:12345 1711180800000 '{"type":"private","peer_uid":67890}'
  ZADD dialogs:12345 1711180700000 '{"type":"group","group_id":10001}'

// 获取对话列表（按时间倒序）
std::vector<DialogInfo> GetDialogList(int64_t uid, int offset, int limit) {
    auto redis = RedisMgr::Instance();
    
    // 获取最近 limit 条对话
    auto results = redis->ZRevRange("dialogs:" + uid, offset, offset + limit - 1);
    
    // 获取每个对话的详情
    std::vector<DialogInfo> dialogs;
    for (const auto& item : results) {
        auto info = GetDialogDetailFromCache(uid, item);
        if (info) dialogs.push_back(*info);
    }
    
    return dialogs;
}

// 更新对话列表（写入 Redis + PostgreSQL 双写）
void UpdateDialogList(int64_t uid, const DialogInfo& dialog) {
    auto redis = RedisMgr::Instance();
    
    // 1. 写入 Redis Sorted Set
    std::string member = BuildDialogMember(dialog);
    redis->ZAdd("dialogs:" + uid, dialog.last_msg_ts, member);
    
    // 2. 设置过期时间（24 小时）
    redis->Expire("dialogs:" + uid, 86400);
    
    // 3. 异步写入 PostgreSQL（通过 Kafka 解耦）
    PublishDialogSyncEvent(uid, dialog);
}
```

#### 4.3.2 消息缓存

```cpp
// 最近消息缓存（用于快速加载）
// Type: String (JSON)
// TTL: 5 分钟
Key: "msg:{msg_id}"
Example: GET msg:msg-uuid-789
Value: '{"from_uid":12345,"to_uid":67890,"content":"Hello!","server_time":1711180800001}'

// 热点消息预加载（消息发送后主动缓存）
void CacheHotMessage(const Message& msg) {
    auto redis = RedisMgr::Instance();
    
    // 1. 缓存消息详情
    redis->Set("msg:" + msg.msg_id, SerializeMessage(msg), 300);  // 5 分钟
    
    // 2. 加入热点消息集合（用于热点探测）
    redis->ZIncrBy("hot_messages", 1, msg.msg_id);
    
    // 3. 定时清理：移除冷数据
    // 移除 score < threshold 的消息
    redis->ZRemRangeByScore("hot_messages", "-inf", GetCurrentTimestamp() - 3600);
}
```

### 4.4 分布式锁实现

#### 4.4.1 分布式锁的使用场景

```cpp
// ChatServer/DistLock.cpp

// 场景 1: Snowflake ID 生成（保证多节点 ID 不冲突）
bool AcquireSnowflakeLock(int64_t datacenter_id, int64_t worker_id) {
    std::string lock_key = "lock:snowflake:" + 
                          std::to_string(datacenter_id) + ":" + 
                          std::to_string(worker_id);
    std::string lock_value = GenerateUUID();
    
    return redis->SetNX(lock_key, lock_value, 10);  // 10 秒超时
}

// 场景 2: 消息发送幂等控制（防止重复发送）
bool AcquireMessageSendLock(const std::string& msg_id, int ttl_seconds) {
    std::string lock_key = "lock:msg_send:" + msg_id;
    return redis->SetNX(lock_key, "1", ttl_seconds);
}

// 场景 3: 用户状态变更（防止并发修改）
bool AcquireUserStateLock(int64_t uid) {
    std::string lock_key = "lock:user_state:" + std::to_string(uid);
    return redis->SetNX(lock_key, "1", 5);  // 5 秒超时
}

// 场景 4: ChatServer 节点注册（选举）
bool ElectLeader(const std::string& node_name) {
    std::string lock_key = "lock:leader:chatserver_cluster";
    std::string value = node_name + ":" + std::to_string(GetCurrentTimestamp());
    
    // SET NX EX 保证原子性
    bool acquired = redis->SetNX(lock_key, value, 30);
    
    if (acquired) {
        // 注册成功，设置 TTL 续期定时器
        ScheduleLockRefresh(lock_key, value, 30);
    }
    
    return acquired;
}
```

#### 4.4.2 安全的锁释放（Lua 脚本）

```cpp
// ChatServer/DistLock.cpp

// 错误方式：GET + DEL 不是原子操作
void WrongReleaseLock(const std::string& lock_key) {
    std::string value = redis->Get(lock_key);
    if (value == my_value) {  // 检查通过
        // 但在这两步之间，另一个进程可能已经获取了锁
        redis->Del(lock_key);  // 可能删除了别人的锁！
    }
}

// 正确方式：Lua 脚本保证原子性
const char* RELEASE_LOCK_SCRIPT = R"(
    if redis.call('GET', KEYS[1]) == ARGV[1] then
        return redis.call('DEL', KEYS[1])
    else
        return 0
    end
)";

bool SafeReleaseLock(const std::string& lock_key, const std::string& lock_value) {
    auto result = redis->Eval(RELEASE_LOCK_SCRIPT, 1, lock_key, lock_value);
    return result == 1;  // 1 = 成功释放，0 = 锁已被其他进程持有
}

// 锁续期（Watch Dog 机制）
void RefreshLock(const std::string& lock_key, const std::string& lock_value) {
    // 使用 Lua 脚本：只有锁持有者才能续期
    const char* REFRESH_LOCK_SCRIPT = R"(
        if redis.call('GET', KEYS[1]) == ARGV[1] then
            return redis.call('EXPIRE', KEYS[1], ARGV[2])
        else
            return 0
        end
    )";
    
    auto result = redis->Eval(REFRESH_LOCK_SCRIPT, 1, lock_key, lock_value, "30");
    return result == 1;
}
```

---

## 五、消息流程端到端详解

### 5.1 用户登录到收发消息全流程

```
客户端 ──► 服务端 ──► 存储 ──► 消息总线 ──► 目标服务端 ──► 客户端

Step 1: 用户登录（HTTP）
┌──────────────────────────────────────────────────────────────┐
│  QML Client                                                  │
│    POST /user_login { email, password }                      │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  GateServer (:8080)                                          │
│  1. Validate credentials against PostgreSQL (user table)     │
│  2. Generate login_ticket (HMAC-SHA256)                      │
│  3. Query StatusServer for ChatServer routing (gRPC)        │
│  4. Store session in Redis: session:{uid}:{device_id}       │
│  5. Return { uid, token, login_ticket, chat_endpoints }     │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  StatusServer (:50052)                                       │
│  1. Record login event                                       │
│  2. Return least-loaded ChatServer node                     │
│  3. Store login state in Redis                              │
└──────────────────────────────────────────────────────────────┘

Step 2: 建立 TCP/QUIC 长连接
┌──────────────────────────────────────────────────────────────┐
│  QML Client                                                  │
│  Connect to ChatServer:8090 (TCP) or :8190 (QUIC)           │
│  Send ID_CHAT_LOGIN { uid, login_ticket, device_id }        │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  ChatServer (:8090)                                          │
│  1. Verify login_ticket (HMAC-SHA256)                        │
│  2. Create session, store in memory                          │
│  3. Update Redis: online:{uid}=1, chatserver:{uid}=xxx      │
│  4. Add to node's online user set                            │
│  5. Subscribe friend presence channels (Redis Pub/Sub)      │
│  6. Send ID_CHAT_LOGIN_RSP { session_id, heartbeat }         │
└──────────────────────────────────────────────────────────────┘

Step 3: 拉取关系引导数据
┌──────────────────────────────────────────────────────────────┐
│  QML Client                                                  │
│  Send ID_GET_RELATION_BOOTSTRAP { uid, seq }                │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  ChatServer                                                  │
│  1. Query PostgreSQL: friend list, group list                │
│  2. Query MongoDB: recent messages                           │
│  3. Build bootstrap data                                     │
│  4. Return ID_GET_RELATION_BOOTSTRAP_RSP                    │
└──────────────────────────────────────────────────────────────┘

Step 4: 发送私聊消息
┌──────────────────────────────────────────────────────────────┐
│  QML Client (用户 A)                                         │
│  Send ID_TEXT_CHAT_MSG { from_uid, to_uid, msg_id, content }│
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  ChatServer-1 (A 所在节点)                                   │
│  1. Validate session, check friend relationship               │
│  2. Generate server_time (Snowflake ID)                     │
│  3. INSERT PostgreSQL: chat_private_msg                      │
│  4. INSERT MongoDB: private_messages                         │
│  5. UPDATE Redis: dialogs:{A} (Sorted Set)                   │
│  6. Check if B is online (Redis: online:{B})                │
│  7. If B online: send ID_NOTIFY_TEXT_CHAT_MSG to B via TCP  │
│  8. If B offline: publish to RabbitMQ (chat.notify.offline) │
│  9. Publish to Kafka (chat.private.v1) for cross-node       │
│  10. Send ID_TEXT_CHAT_MSG_RSP to A                         │
└──────────────────────────────────────────────────────────────┘
```

### 5.2 消息不丢不重的保障机制

```
发送方保障（不丢）：

┌─────────────────────────────────────────────────────────────┐
│                      ChatServer                              │
│                                                              │
│  1. 写存储后再 ACK                                            │
│     Send() {                                                 │
│       INSERT MongoDB ──► INSERT PostgreSQL ──► ACK Client   │
│       │                                                       │
│       │ 如果 INSERT 失败，返回错误，让客户端重试                │
│       ▼                                                       │
│     }                                                        │
│                                                              │
│  2. Kafka 持久化                                              │
│     Produce() {                                              │
│       brokers = [b1, b2, b3]                                 │
│       acks = all                                             │
│       enable_idempotence = true                              │
│       // 等待所有 ISR 确认后才返回                             │
│     }                                                        │
│                                                              │
│  3. RabbitMQ 可靠投递                                         │
│     Publish() {                                              │
│       durable = true                                         │
│       persistent = true                                      │
│       // 消息刷到磁盘                                         │
│     }                                                        │
└─────────────────────────────────────────────────────────────┘

发送方保障（不重）：

┌─────────────────────────────────────────────────────────────┐
│                        幂等设计                              │
│                                                              │
│  1. 客户端生成 msg_id (UUID)                                  │
│     msg_id = UUID.randomUUID()  // 全局唯一                   │
│     // 同一 msg_id 的重试请求会被服务端去重                    │
│                                                              │
│  2. PostgreSQL UNIQUE 约束                                   │
│     INSERT ... ON CONFLICT (msg_id) DO NOTHING              │
│     // 重复插入被约束拒绝                                     │
│                                                              │
│  3. Kafka 幂等生产者                                         │
│     ProducerID + SequenceNumber ──► 唯一标识                 │
│     // Broker 自动去重                                        │
│                                                              │
│  4. RabbitMQ 手动 ACK                                        │
│     // 只有处理成功后 ACK，失败不 ACK 会重试                   │
│     // 但需要业务端幂等（检查 msg_id 是否已处理）               │
└─────────────────────────────────────────────────────────────┘

接收方保障（可靠投递）：

┌─────────────────────────────────────────────────────────────┐
│                        推送机制                              │
│                                                              │
│  1. TCP/QUIC 可靠传输                                        │
│     // ARQ 自动重传，丢包重发                                 │
│                                                              │
│  2. 推送 + ACK 确认                                          │
│     Server ──Push──► Client                                   │
│                  │                                           │
│                  │ ACK (30 秒内)                             │
│                  ▼                                           │
│           未收到 ACK ──► 重试（最多 3 次）                     │
│                                                              │
│  3. 离线消息补拉                                              │
│     Client ──Pull──► Server                                   │
│     // 拉取离线期间的所有消息                                  │
└─────────────────────────────────────────────────────────────┘
```

### 5.3 消息顺序性保障

```
单聊消息顺序（严格有序）：

┌─────────────────────────────────────────────────────────────┐
│  同一会话的所有消息按 server_time 严格排序                     │
│                                                              │
│  用户 A ──► 用户 B                                            │
│                                                              │
│  消息 1 (msg_id: "001") ──► ChatServer ──► server_time: 1001 │
│  消息 2 (msg_id: "002") ──► ChatServer ──► server_time: 1002 │
│  消息 3 (msg_id: "003") ──► ChatServer ──► server_time: 1003 │
│                                                              │
│  // Kafka 按 Partition Key 保证同一会话消息落在同一 Partition  │
│  // 同一 Partition 内有序，消息顺序由 Kafka 保证              │
│  // 消费时按 offset 顺序消费                                  │
└─────────────────────────────────────────────────────────────┘

群聊消息顺序（在群内有序）：

┌─────────────────────────────────────────────────────────────┐
│  同一群的所有消息按 group_seq 严格排序                         │
│                                                              │
│  群 G ──► 成员 [A, B, C, D]                                  │
│                                                              │
│  成员 X 发送消息 ──► ChatServer 分配 group_seq ──► 广播     │
│                                                              │
│  group_seq = 全局原子递增计数器                               │
│  // 不同群的消息可以乱序                                      │
│  // 同一群的消息必须严格按 group_seq 排序                     │
└─────────────────────────────────────────────────────────────┘

跨会话消息顺序（最终一致）：

┌─────────────────────────────────────────────────────────────┐
│  不同会话的消息按时间倒序展示，允许轻微乱序                     │
│                                                              │
│  对话列表:                                                   │
│  ┌────────────────────────────────────────────┐             │
│  │ 对话 A ── 最后消息时间: 10:30:01             │             │
│  │ 对话 B ── 最后消息时间: 10:30:00             │             │
│  │ 对话 C ── 最后消息时间: 10:29:55             │             │
│  └────────────────────────────────────────────┘             │
│                                                              │
│  // 因为网络延迟，对话 A 的消息可能实际比对话 B 早到           │
│  // 但最终以服务端确认的 last_msg_time 为准                    │
└─────────────────────────────────────────────────────────────┘
```

---

## 六、消息系统运维指南

### 6.1 监控指标

#### 6.1.1 Kafka 关键指标

| 指标名称 | 描述 | 正常范围 | 告警阈值 |
|----------|------|----------|----------|
| `kafka_consumer_lag` | 消费者落后于生产者的消息数 | < 1000 | > 10000 |
| `kafka_under_replicated_partitions` | 复制不足的分区数 | 0 | > 0 |
| `kafka_offline_partitions` | 离线分区数 | 0 | > 0 |
| `kafka_messages_per_second` | 每秒消息数 | 1000-10000 | - |
| `kafka_consumer_fetch_rate` | 消费者拉取速率 | > 100/s | < 10/s |

#### 6.1.2 RabbitMQ 关键指标

| 指标名称 | 描述 | 正常范围 | 告警阈值 |
|----------|------|----------|----------|
| `rabbitmq_queue_depth` | 队列积压消息数 | < 1000 | > 10000 |
| `rabbitmq_consumers` | 消费者数量 | > 0 | = 0 |
| `rabbitmq_publish_rate` | 发布速率 | 100-1000/s | - |
| `rabbitmq_consume_rate` | 消费速率 | 100-1000/s | - |
| `rabbitmq_dlq_depth` | 死信队列深度 | 0 | > 100 |

#### 6.1.3 Redis 关键指标

| 指标名称 | 描述 | 正常范围 | 告警阈值 |
|----------|------|----------|----------|
| `redis_connected_clients` | 连接数 | < 1000 | > 5000 |
| `redis_memory_used` | 内存使用 | < 80% | > 90% |
| `redis_ops_per_second` | 每秒操作数 | 10000-100000 | < 1000 |
| `redis_keyspace_hits` | Key 命中率 | > 95% | < 90% |

### 6.2 故障排查

#### 6.2.1 消息延迟排查

```
排查路径：

1. 检查 Kafka Consumer Lag
   > kafka-consumer-groups.sh --bootstrap-server 127.0.0.1:9092 
     --group chatserver-cluster --describe
   
   // 如果 LAG > 10000，说明消费者处理速度跟不上

2. 检查消费者处理时间
   // 在消费者代码中埋点，记录每条消息的处理时长
   auto start = std::chrono::steady_clock::now();
   ProcessMessage(msg);
   auto elapsed = std::chrono::duration_cast<std::milliseconds>(
       std::chrono::steady_clock::now() - start).count();
   LOG_INFO("message_processing_ms={}", elapsed);

3. 检查 Kafka 网络延迟
   // 在生产者端记录发送延迟
   auto start = std::chrono::steady_clock::now();
   producer->Produce(topic, partition, key, value);
   auto elapsed = std::chrono::duration_cast<std::milliseconds>(...);
   LOG_INFO("kafka_produce_latency_ms={}", elapsed);

4. 检查消费者线程池瓶颈
   // 如果消息处理涉及 IO（数据库写入），可能是线程池耗尽
   // 增加 max_poll_records 或消费者线程数
```

#### 6.2.2 消息丢失排查

```
排查路径：

1. 检查 Kafka 生产者确认
   // 如果 acks=1，Leader 写入后宕机会丢消息
   // 确认 acks=all 且 enable_idempotence=true

2. 检查 PostgreSQL/MongoDB 写入是否成功
   // 在写入后检查返回值，不要假设写入成功

3. 检查消费者 ACK 逻辑
   // 确保消息处理成功后才 ACK
   // 不要在处理失败时错误地 ACK

4. 检查 RabbitMQ 队列是否满
   // 如果队列满了，新消息会被拒绝
   // 增加队列容量或消费者数量

5. 检查 Redis 持久化
   // 如果 Redis 内存不足，会触发淘汰策略
   // 确保 maxmemory 足够且有持久化策略
```

---

## 七、Topic 初始化脚本

### 7.1 Kafka Topic 初始化

```powershell
# scripts/init_kafka_topics.ps1

$kafkaBrokers = "127.0.0.1:19092"
$topics = @(
    @{
        Name = "chat.private.v1"
        Partitions = 8
        ReplicationFactor = 3
        Config = @{
            "retention.ms" = "604800000"  # 7 天
            "cleanup.policy" = "delete"
            "min.insync.replicas" = "2"
        }
    },
    @{
        Name = "chat.group.v1"
        Partitions = 8
        ReplicationFactor = 3
        Config = @{
            "retention.ms" = "604800000"
            "cleanup.policy" = "delete"
            "min.insync.replicas" = "2"
        }
    },
    @{
        Name = "dialog.sync.v1"
        Partitions = 8
        ReplicationFactor = 3
        Config = @{
            "retention.ms" = "259200000"  # 3 天
            "cleanup.policy" = "delete"
        }
    },
    @{
        Name = "relation.state.v1"
        Partitions = 4
        ReplicationFactor = 3
        Config = @{
            "retention.ms" = "604800000"
        }
    },
    @{
        Name = "user.profile.changed.v1"
        Partitions = 4
        ReplicationFactor = 3
        Config = @{
            "retention.ms" = "259200000"
        }
    },
    @{
        Name = "audit.login.v1"
        Partitions = 2
        ReplicationFactor = 3
        Config = @{
            "retention.ms" = "2592000000"  # 30 天
        }
    }
)

foreach ($topic in $topics) {
    $configStr = $topic.Config.GetEnumerator() | 
                 ForEach-Object { "$($_.Key)=$($_.Value)" } | 
                 Join-String -Separator ","
    
    $cmd = "kafka-topics.sh --create --bootstrap-server $kafkaBrokers " +
           "--topic $($topic.Name) --partitions $($topic.Partitions) " +
           "--replication-factor $($topic.ReplicationFactor) " +
           "--config $configStr"
    
    Write-Host "Creating topic: $($topic.Name)"
    Invoke-Expression $cmd
}
```

### 7.2 RabbitMQ 拓扑初始化

```powershell
# scripts/init_rabbitmq_topology.ps1

$rabbitmqHost = "127.0.0.1"
$rabbitmqPort = 5672
$rabbitmqUser = "guest"
$rabbitmqPass = "guest"

# 定义交换机和队列拓扑
$topology = @{
    "memochat.direct" = @{
        Type = "direct"
        Durable = $true
        Queues = @(
            @{
                Name = "chat_notify_queue"
                RoutingKey = "chat.notify"
                Durable = $true
                Args = @{ "x-max-priority" = 5 }
            },
            @{
                Name = "chat_retry_queue"
                RoutingKey = "chat.retry"
                Durable = $true
                Args = @{ "x-max-priority" = 10 }
            },
            @{
                Name = "relation_notify_queue"
                RoutingKey = "relation.notify"
                Durable = $true
            },
            @{
                Name = "verify_email_queue"
                RoutingKey = "verify.email"
                Durable = $true
            },
            @{
                Name = "gate_cache_queue"
                RoutingKey = "gate.cache"
                Durable = $true
            },
            @{
                Name = "status_presence_queue"
                RoutingKey = "status.presence"
                Durable = $true
            }
        )
    }
    "memochat.dlx" = @{
        Type = "fanout"
        Durable = $true
        Queues = @(
            @{
                Name = "memochat_dlq"
                RoutingKey = ""
                Durable = $true
            }
        )
    }
}

foreach ($exchange in $topology.Keys) {
    $config = $topology[$exchange]
    
    # 创建交换机
    Write-Host "Creating exchange: $exchange"
    # rabbitmqadmin 命令或 HTTP API 调用
    
    foreach ($queue in $config.Queues) {
        Write-Host "Creating queue: $($queue.Name)"
        # 绑定队列到交换机
    }
}
```
