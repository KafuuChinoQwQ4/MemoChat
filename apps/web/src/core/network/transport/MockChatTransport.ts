/**
 * MockChatTransport — same interface as WebSocketChatTransport.
 *
 * Enabled via VITE_USE_MOCK_TRANSPORT=1.
 * Replays a queue of ArrayBuffer frames (fixed test fixtures) as if they arrived
 * from the real bridge. Allows full chat feature development without a live server.
 */
import { encodeChatFrame } from "@/core/network/codec/ChatFrameCodec"
import { ChatFrameParser } from "@/core/network/codec/ChatFrameParser"
import { ReqId } from "@/core/network/opcodes/reqIds"
import type { ChatTransport } from "./ChatTransport"
import type { ServerInfo } from "./transport.types"

export class MockChatTransport implements ChatTransport {
  private _parser = new ChatFrameParser()
  private _connected = false

  onReady?: () => void
  onMessage?: (reqId: ReqId, payload: string) => void
  onClose?: (reason?: string) => void
  onError?: (err: unknown) => void

  constructor(
    /** Optional frame queue to replay after chat-login ack. */
    private readonly _replayFrames: ArrayBuffer[] = [],
    /** Simulated network round-trip delay in ms */
    private readonly _delayMs = 80,
  ) {}

  connect(_info: ServerInfo): void {
    this._connected = false
    setTimeout(() => {
      // Simulate chat-login ack
      const loginRsp = encodeChatFrame(ReqId.ID_CHAT_LOGIN_RSP, JSON.stringify({ error: 0 }))
      const frames = this._parser.append(loginRsp)
      for (const f of frames) {
        if (f.reqId === ReqId.ID_CHAT_LOGIN_RSP) {
          this._connected = true
          this.onReady?.()
        }
        this.onMessage?.(f.reqId, f.payload)
      }
      // Replay any pre-loaded frames
      for (const buf of this._replayFrames) {
        setTimeout(() => {
          for (const f of this._parser.append(buf)) {
            this.onMessage?.(f.reqId, f.payload)
          }
        }, this._delayMs)
      }
    }, this._delayMs)
  }

  send(_reqId: ReqId, _payload: string): void {
    // Swallow sends in mock mode; individual tests can spy on this method.
  }

  close(): void {
    this._connected = false
    this.onClose?.("mock_closed")
  }

  isConnected(): boolean { return this._connected }

  /**
   * Inject a frame as if it arrived from the server.
   * Useful in test setup to drive specific states.
   */
  injectFrame(reqId: ReqId, payload: string): void {
    const buf = encodeChatFrame(reqId, payload)
    for (const f of this._parser.append(buf)) {
      this.onMessage?.(f.reqId, f.payload)
    }
  }
}
