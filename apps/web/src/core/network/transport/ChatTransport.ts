/**
 * ChatTransport interface — mirrors IChatTransport.h
 * No EventEmitter: callbacks are injected via onXxx properties.
 */
import type { ReqId } from "@/core/network/opcodes/reqIds"
import type { ServerInfo } from "./transport.types"

export interface ChatTransport {
  /** Attempt to connect to the WS bridge */
  connect(info: ServerInfo): void
  /** Send an opcode frame with JSON payload */
  send(reqId: ReqId, payload: string): void
  /** Gracefully close the connection */
  close(): void
  /** True when the transport is in "ready" state */
  isConnected(): boolean

  // Callback hooks — set these before calling connect()
  /** Called when the connection reaches "ready" (after chat-login ack) */
  onReady?: () => void
  /** Called for each decoded incoming frame */
  onMessage?: (reqId: ReqId, payload: string) => void
  /** Called on graceful or error close */
  onClose?: (reason?: string) => void
  /** Called on connection error */
  onError?: (err: unknown) => void
}
