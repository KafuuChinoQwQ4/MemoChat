# 登录失败问题排查报告

## 问题现象

客户端登录时显示「聊天服务登录失败（错误码:1）」，但服务端日志显示 `chat.login.succeeded`，即服务端登录已成功。

## 错误码含义

错误码 1 在客户端对应 `ErrorCodes::ERR_JSON`（JSON 解析失败），定义在 `client/MemoChatShared/global.h`：

```cpp
enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1,
    ...
}
```

## 根因分析

**客户端 TCP/QUIC 消息解析存在 Bug**。

在 `tcpmgr.cpp`（TCP 传输）和 `QuicChatTransport.cpp`（QUIC 传输）中，接收消息时的处理顺序为：

1. 从接收缓冲区读取消息头（msg_id + msg_len）
2. 使用 `QByteArray::fromRawData` **零拷贝**引用缓冲区中的消息体
3. 调用 `memmove` 将缓冲区前移，为下一帧腾出空间
4. 触发 `handleMsg` 派发消息

问题出在 **第 2 步**：当同一批次 `readyRead` 中已缓冲了多个帧时（例如登录响应 1006 后面紧跟着心跳响应或其他推送），`memmove` 会**在 `handleMsg` 执行前覆盖当前帧的正文**，导致 JSON 解析失败。

```
原始数据: [1006响应][心跳响应][...]
            ↑ 被覆盖
```

服务端日志显示 `chat.login.succeeded` 证明响应已发送成功，客户端只是解析时缓冲区已被后续数据破坏。

## 修复方案

将 `QByteArray::fromRawData`（零拷贝引用）改为深拷贝，在 `memmove` 之前复制消息体：

```cpp
// tcpmgr.cpp
// 修改前：
const QByteArray messageBody = QByteArray::fromRawData(_buffer.constData(), _message_len);
::memmove(_buffer.data(), _buffer.data() + _message_len, ...);

// 修改后：
const QByteArray messageBody(_buffer.constData(), _message_len);
::memmove(_buffer.data(), _buffer.data() + _message_len, ...);
```

`QuicChatTransport.cpp` 做了同样的修改。

## 修改文件

- `client/MemoChatShared/tcpmgr.cpp`
- `client/MemoChatShared/QuicChatTransport.cpp`

## 验证

本地编译通过（Debug 和 Release），重新运行客户端后登录成功。

## 相关日志片段

服务端（已成功）：
```
{"event":"chat.login.succeeded","message":"chat login success",...}
```

客户端 Qt 日志（解析失败）：
```
chat login rsp json parse failed
```

---

**附：关于日志中的 `heartbeat expired` / `EOF`**

这是登录成功后客户端连接断开或心跳超时导致的，和本次错误码 1 的根因不同。修复 JSON 解析后，若仍有心跳超时问题，需单独排查客户端心跳发送与连接保持逻辑。
