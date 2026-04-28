# MemoChat QUIC + TCP 回退部署计划

> 当前版本：2026-04-26  
> 当前代码路径：客户端在 `apps/client/desktop`，ChatServer 在 `apps/server/core/ChatServer`。TCP 端口为 `8090-8093`，QUIC 端口为 `8190-8193`。

## 目标

使用 QUIC 作为首选的客户端到聊天传输层，同时保持 TCP 作为自动回退路径。

客户端传输层已经抽象为：

- `IChatTransport`
- `ChatMessageDispatcher`
- `TransportSelector`
- `TcpMgr`
- `QuicChatTransport`

## 当前状态

客户端选择器行为已经接线完成：

1. 解析 `preferred_transport`
2. 解析 `fallback_transport`
3. 解析 `chat_endpoints[].transport`
4. 首先尝试首选传输层
5. 如果首选传输层失败，回退到配置的回退传输层

当前代码位置：

- `apps/client/desktop/MemoChatShared/global.h`
- `apps/client/desktop/MemoChatShared/IChatTransport.h`
- `apps/client/desktop/MemoChatShared/QuicChatTransport.*`
- `apps/client/desktop/MemoChatShared/tcpmgr.*`
- `apps/client/desktop/MemoChatShared/ChatMessageDispatcher.*`
- `apps/client/desktop/MemoChat-qml/TransportSelector.*`
- `apps/client/desktop/MemoChat-qml/AppControllerSession.cpp`

## 目标协议

Gate 登录响应应提供支持传输的端点。

示例：

```json
{
  "protocol_version": 3,
  "preferred_transport": "quic",
  "fallback_transport": "tcp",
  "chat_endpoints": [
    {
      "transport": "quic",
      "host": "127.0.0.1",
      "port": "8192",
      "server_name": "chat-1",
      "priority": 0
    },
    {
      "transport": "tcp",
      "host": "127.0.0.1",
      "port": "8090",
      "server_name": "chat-1",
      "priority": 1
    }
  ]
}
```

## 客户端工作

### 第一阶段

- 在 QUIC 之上保持现有的消息封装格式
- 重用当前的应用程序消息 ID 和 JSON 负载
- 在落地 QUIC 时不要重新设计业务负载

### 第二阶段

在 `QuicChatTransport` 中实现真正的 MsQuic 支持的客户端传输。

必需工作：

- 打开 MsQuic API 和注册
- 创建客户端配置和凭证设置
- 为每个聊天会话打开一个连接
- 打开一个双向流用于应用程序流量
- 使用现有的 `ReqId + len + payload` 形状封装传出数据包
- 在派发到 `ChatMessageDispatcher` 之前重新组装传入流数据

### 第三阶段

改进选择器行为：

- 区分握手超时与传输不可用与服务器拒绝
- 为短本地缓存窗口持久化最后成功的传输选择
- 避免在同一次登录会话内重复 QUIC/TCP 振荡

## 服务端工作

### ChatServer

添加 QUIC 监听器和会话适配器。

必需形状：

- `QuicChatSession`
- `QuicChatServer`
- 与当前 TCP 会话相同的业务消息入口

不要为 QUIC 复制聊天业务处理程序。

TCP 和 QUIC 都应使用相同的消息处理路径。

### GateServer

扩展登录响应生成：

- 按环境选择首选传输层
- 包含支持传输的端点
- 保持回退传输层明确

## 部署顺序

1. 落地 MsQuic 依赖和编译守卫
2. 实现客户端 QUIC 握手
3. 实现 ChatServer QUIC 监听器
4. 启用 GateServer 支持传输的端点响应
5. 通过 QUIC 运行本地端到端登录
6. 验证 QUIC 失败时自动回退到 TCP

## 验证清单

- QUIC 端点健康时 QUIC 登录成功
- QUIC 端点缺失时 TCP 回退成功
- 通过 QUIC 发送/接收消息正常工作
- 心跳路径通过 QUIC 正常工作
- 重连路径重用选择器而不是绕过它
- 调度器行为保持传输无关

## 首次落地的非目标

- HTTP/3 迁移
- 多流应用程序多路复用
- 通过 QUIC 传输媒体
- 一次性替换所有服务端 TCP 路径
