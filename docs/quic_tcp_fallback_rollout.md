# MemoChat QUIC + TCP Fallback Rollout

## Goal

Use QUIC as the preferred client-to-chat transport while keeping TCP as the automatic fallback path.

The client transport layer is already abstracted as:

- `IChatTransport`
- `ChatMessageDispatcher`
- `TransportSelector`
- `TcpMgr`
- `QuicChatTransport`

## Current State

Client-side selector behavior is already wired:

1. Parse `preferred_transport`
2. Parse `fallback_transport`
3. Parse `chat_endpoints[].transport`
4. Try preferred transport first
5. If preferred transport fails, fall back to the configured fallback transport

Current code locations:

- `client/MemoChatShared/global.h`
- `client/MemoChatShared/IChatTransport.h`
- `client/MemoChatShared/QuicChatTransport.*`
- `client/MemoChatShared/tcpmgr.*`
- `client/MemoChatShared/ChatMessageDispatcher.*`
- `client/MemoChat-qml/TransportSelector.*`
- `client/MemoChat-qml/AppControllerSession.cpp`

## Target Contract

Gate login response should provide transport-aware endpoints.

Example:

```json
{
  "protocol_version": 3,
  "preferred_transport": "quic",
  "fallback_transport": "tcp",
  "chat_endpoints": [
    {
      "transport": "quic",
      "host": "127.0.0.1",
      "port": "8092",
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

## Client Work

### Phase 1

- Keep the existing message framing format on top of QUIC
- Reuse the current application message IDs and JSON payloads
- Do not redesign business payloads while landing QUIC

### Phase 2

Implement real MsQuic-backed client transport in `QuicChatTransport`.

Required work:

- Open MsQuic API and registration
- Create client configuration and credential setup
- Open one connection per chat session
- Open one bidirectional stream for app traffic
- Frame outgoing packets using the existing `ReqId + len + payload` shape
- Reassemble incoming stream data before dispatching to `ChatMessageDispatcher`

### Phase 3

Improve selector behavior:

- Distinguish handshake timeout vs. transport unavailable vs. server reject
- Persist last successful transport choice for a short local cache window
- Avoid repeated QUIC/TCP oscillation within the same login session

## Server Work

### ChatServer

Add a QUIC listener and session adapter.

Required shape:

- `QuicChatSession`
- `QuicChatServer`
- Same business message ingress as current TCP sessions

Do not duplicate chat business handlers for QUIC.

TCP and QUIC should both feed the same message-processing path.

### GateServer

Extend login response generation:

- choose preferred transport per environment
- include transport-aware endpoints
- keep fallback transport explicit

## Rollout Order

1. Land MsQuic dependency and compile guards
2. Implement client QUIC handshake
3. Implement ChatServer QUIC listener
4. Enable GateServer transport-aware endpoint response
5. Run local end-to-end login through QUIC
6. Verify automatic fallback to TCP when QUIC fails

## Verification Checklist

- QUIC login succeeds when QUIC endpoint is healthy
- TCP fallback succeeds when QUIC endpoint is missing
- Message send/receive works over QUIC
- Heartbeat path works over QUIC
- Reconnect path reuses the selector instead of bypassing it
- Dispatcher behavior stays transport-agnostic

## Non-Goals For First Landing

- HTTP/3 migration
- Multi-stream app multiplexing
- Media transport over QUIC
- Replacing all server-side TCP paths at once
