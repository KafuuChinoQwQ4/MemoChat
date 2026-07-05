# _deferred/call — 通话功能（推迟）

v1 不实现。以下是扩展位说明。

## 预留 opcode

| opcode | 说明 |
|--------|------|
| 1085   | ID_NOTIFY_CALL_EVENT_REQ |
| 1086   | ID_CALL_START |
| 1087   | ID_CALL_ACCEPT |
| 1088   | ID_CALL_REJECT |
| 1089   | ID_CALL_CANCEL |
| 1090   | ID_CALL_HANGUP |
| 1091   | ID_CALL_GET_TOKEN |

## 实现计划（后续版本）

- 使用 LiveKit SDK (browser-native WebRTC)
- 与桌面端使用同一 chat-server WebSocket 信令通道
- 将语音/视频通话 UI 实现为浮层 Modal（不打断消息流）
