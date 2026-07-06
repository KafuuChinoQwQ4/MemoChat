# QUIC 导致 ChatServer 崩溃的排障记录

## 背景

2026-07-06，本地开启 QUIC 后，桌面客户端表现为：

- HTTP 登录成功，进入聊天主界面。
- 会话列表和好友 bootstrap 长时间加载不出来。
- 过一会 chat connection closed，客户端自动回到登录页。
- 客户端日志里能看到 QUIC 失败后尝试 TCP fallback，但 TCP `127.0.0.1:8090` 返回 `Connection refused`。

这类现象容易误判成客户端没有 fallback。实际根因在服务端 QUIC 路径。

## 证据链

客户端日志关键顺序：

1. `/user_login` 成功，服务端返回 `chat_endpoints`，其中 QUIC 指向 `127.0.0.1:8190`，TCP 指向 `127.0.0.1:8090`。
2. `TransportSelector` 先激活 QUIC，客户端连接 `8190` 并发送 chat login。
3. chat connection closed。
4. 客户端开始重连，并切到 TCP `8090`。
5. TCP 连接失败：`Connection refused`。
6. 重连耗尽后切回登录页。

服务端现场：

```bash
pgrep -af ChatServer
ss -ltnup | rg '(:8090|:8190)\b|Process'
dmesg --ctime | rg 'ChatServer' | tail -20
```

异常时 `8090/8190` 没有 `ChatServer` listener，`dmesg` 能看到 `ChatServer general protection fault`。这说明 TCP fallback 已经执行，只是 fallback 时 `ChatServer` 进程已经崩溃。

## 根因

崩溃点在 `apps/server/core/ChatServer/transport/QuicChatServer.cpp` 的 `QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED` 分支。

peer-started stream 是客户端已经打开的 stream。服务端只需要接管 callback 和上下文，不应该再调用：

```cpp
StreamStart(ctx->stream, QUIC_STREAM_START_FLAG_IMMEDIATE)
```

原实现还在 `StreamStart` 失败路径里提前删除 `stream_callback_context`，但 MsQuic 仍持有这个 stream callback context。后续 shutdown 或 stream callback 到达时，会访问已经释放的上下文，触发 use-after-free 风险并导致 `ChatServer` 崩溃。

## 修复方案

修复位置：

- `apps/server/core/ChatServer/transport/QuicChatServer.cpp`
- `tests/python/apps/server/core/ChatServer/test_quic_lifetime_contract.py`

服务端处理 peer-started stream 时保留：

- `SetCallbackHandler(ctx->stream, ...)`
- `flushPendingSends(ctx->stream_context.get())`

同时移除：

- `StreamStart(ctx->stream, QUIC_STREAM_START_FLAG_IMMEDIATE)`
- 该失败分支里的 `delete stream_callback_context`
- 该失败分支里清空 stream/session 并关闭 session 的逻辑

当前合同测试锁定：

- `PEER_STREAM_STARTED` 分支必须设置 stream callback。
- `PEER_STREAM_STARTED` 分支必须 flush pending sends。
- `PEER_STREAM_STARTED` 分支不能包含 `StreamStart(ctx->stream`。
- `PEER_STREAM_STARTED` 分支不能包含 `delete stream_callback_context`。

## 验证命令

最小合同测试：

```bash
PYTHONPATH=. python -m unittest \
  tests.python.apps.server.core.ChatServer.test_quic_lifetime_contract \
  tests.python.apps.server.core.ChatServer.test_chatserver_structure
```

重新构建 ChatServer：

```bash
source /root/.memochat-linux-env
cmake --build --preset linux-full-gcc16 --target ChatServer --parallel 12
```

开启 QUIC 重启本地服务：

```bash
source /root/.memochat-linux-env
MEMOCHAT_ENABLE_QUIC=1 MEMOCHAT_START_GPT_SOVITS=0 tools/scripts/status/stop-all-services.sh
MEMOCHAT_ENABLE_QUIC=1 MEMOCHAT_START_GPT_SOVITS=0 tools/scripts/status/start-all-services.sh --skip-gpt-sovits
```

确认监听：

```bash
pgrep -af ChatServer
ss -ltnup | rg '(:8090|:8190)\b|Process'
```

完整聊天 smoke：

```bash
source /root/.memochat-linux-env
python tools/scripts/dev/runtime_smoke_full_chat.py \
  --gate-url https://127.0.0.1:8443 \
  --insecure-tls \
  --timeout 8 \
  --minio-access-key memochat_admin \
  --minio-secret-key 'MinioPass2026!'
```

成功标准：

- `relation_bootstrap` 为 `PASS`。
- `dialog_list` 为 `PASS`。
- QUIC 登录和 QUIC relation bootstrap 成功。
- 探针后 `ChatServer` 仍存活。
- `8090` TCP 和 `8190` UDP 仍由 `ChatServer` 监听。
- `dmesg` 没有新增 `ChatServer general protection fault`。

## 排障判断规则

遇到“开启 QUIC 后会话/好友加载失败并回到登录页”时，先不要直接改客户端 fallback。按顺序确认：

1. 客户端是否真的尝试了 TCP endpoint。
2. TCP 失败是超时、认证失败，还是 `Connection refused`。
3. `ChatServer` 是否还在运行。
4. `8090/8190` 是否还有 listener。
5. `dmesg` 或服务日志是否出现 `ChatServer` 崩溃。

如果 TCP fallback 失败时 `ChatServer` 已经消失，问题属于服务端崩溃，客户端 fallback 只是暴露了进程已经不存在。
