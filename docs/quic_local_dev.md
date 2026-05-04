# QUIC 本地开发指南

> 当前版本：2026-04-26

MemoChat 聊天链路支持 TCP 和 QUIC。客户端通过 `TransportSelector` 选择传输，QUIC 不可用时回退 TCP。ChatServer 的 QUIC 端口为 `8190-8193`，TCP 端口为 `8090-8093`。

## 1. 相关代码

客户端：

```text
apps/client/desktop/MemoChatShared/IChatTransport.h
apps/client/desktop/MemoChatShared/QuicChatTransport.*
apps/client/desktop/MemoChatShared/tcpmgr.*
apps/client/desktop/MemoChat-qml/TransportSelector.*
```

服务端：

```text
apps/server/core/ChatServer/QuicChatServer.*
apps/server/core/ChatServer/CServer.*
apps/server/core/ChatServer/CSession.*
```

配置：

```text
apps/server/core/ChatServer/config.ini
apps/server/core/ChatServer/chatserver1.ini
apps/server/core/ChatServer/chatserver2.ini
apps/server/core/ChatServer/chatserver3.ini
apps/server/core/ChatServer/chatserver4.ini
```

## 2. 构建

本地构建统一使用 full 预设：

```powershell
cmake --preset msvc2022-full
cmake --build --preset msvc2022-full
```

msquic 依赖通过当前 full 构建配置和 vcpkg 环境提供。旧的 msquic 专用 preset 已不再作为本地入口。

## 3. 开发证书

生成证书：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\scripts\windows\new_dev_quic_cert.ps1
```

配置示例：

```ini
[Quic]
Alpn=memochat-chat
PfxFile=<repo-root>\artifacts\certs\quic-dev\server-schannel3.pfx
PfxPassword=memo
CertFile=<repo-root>\artifacts\certs\quic-dev\server-cert.pem
PrivateKeyFile=<repo-root>\artifacts\certs\quic-dev\server-key.pem
```

Windows SChannel 优先使用 PFX；OpenSSL/ngtcp2 场景使用 PEM。

## 4. 启动

部署运行时文件：

```powershell
tools\scripts\status\deploy_services.bat
```

启动服务：

```powershell
tools\scripts\status\start-all-services.bat
```

检查端口：

```powershell
netstat -ano | findstr "8190 8191 8192 8193"
```

## 5. 回退策略

客户端应遵循：

1. 读取 GateServer 登录响应中的 `preferred_transport` 和 `fallback_transport`。
2. 优先连接 QUIC。
3. QUIC 握手、证书、ALPN、流创建失败时自动回退 TCP。
4. TCP 成功后仍使用同一套二进制帧协议和 ChatServer 登录 ticket。

## 6. 排障

| 问题 | 检查 |
|------|------|
| QUIC 握手失败 | PFX/PEM 路径、密码、ALPN 是否一致 |
| 客户端直接回退 TCP | msquic DLL 是否部署、服务端 QUIC 端口是否监听 |
| 能连上但登录失败 | GateServer 和 ChatServer 的 `ChatAuth.HmacSecret` 是否一致 |
| HTTP/3 失败 | HTTP/3 网关和聊天 QUIC 是两条链路，分别检查 |

注意：GateServer HTTP/3 只影响 HTTP API，不等同于 ChatServer QUIC 聊天长连接。
