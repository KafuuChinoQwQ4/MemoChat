/** Fixed frames for MockChatTransport replay in tests */
import { ReqId } from "@/core/network/opcodes/reqIds"
import { encodeChatFrame } from "@/core/network/codec/ChatFrameCodec"

/** Minimal chat-login success response frame (opcode 1006) */
export const FRAME_CHAT_LOGIN_RSP = encodeChatFrame(
  ReqId.ID_CHAT_LOGIN_RSP,
  JSON.stringify({ error: 0 }),
)

/** Heartbeat ack frame (opcode 1024) */
export const FRAME_HEARTBEAT_RSP = encodeChatFrame(
  ReqId.ID_HEARTBEAT_RSP,
  JSON.stringify({}),
)

/** Bootstrap relation response frame (opcode 1093) */
export const FRAME_RELATION_BOOTSTRAP_RSP = encodeChatFrame(
  ReqId.ID_GET_RELATION_BOOTSTRAP_RSP,
  JSON.stringify({ error: 0, friend_list: [], apply_list: [] }),
)
