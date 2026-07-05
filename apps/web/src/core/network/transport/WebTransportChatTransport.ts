/**
 * WebTransportChatTransport — browser WebTransport chat transport.
 *
 * Uses one reliable bidirectional stream carrying the same uint16+uint16+JSON
 * chat frames as WebSocket and desktop TCP.
 */
import { encodeChatFrame } from "@/core/network/codec/ChatFrameCodec"
import { ChatFrameParser } from "@/core/network/codec/ChatFrameParser"
import {
  HEARTBEAT_INTERVAL_MS,
  HEARTBEAT_MAX_MISS,
  RECONNECT_BASE_DELAY_MS,
} from "@/core/network/opcodes/protocol"
import { ReqId } from "@/core/network/opcodes/reqIds"
import type { ChatTransport } from "./ChatTransport"
import type { ServerInfo } from "./transport.types"

interface WebTransportBidirectionalStreamLike {
  readable: ReadableStream<Uint8Array>
  writable: WritableStream<Uint8Array>
}

interface WebTransportClientLike {
  readonly ready: Promise<void>
  readonly closed: Promise<unknown>
  createBidirectionalStream(): Promise<WebTransportBidirectionalStreamLike>
  close(closeInfo?: { closeCode?: number; reason?: string }): void
}

interface WebTransportConstructorLike {
  new (
    url: string,
    options?: {
      serverCertificateHashes?: Array<{ algorithm: string; value: BufferSource }>
    },
  ): WebTransportClientLike
}

export function getWebTransportConstructor(): WebTransportConstructorLike | undefined {
  return (globalThis as typeof globalThis & { WebTransport?: WebTransportConstructorLike })
    .WebTransport
}

export class WebTransportChatTransport implements ChatTransport {
  private _transport: WebTransportClientLike | null = null
  private _writer: WritableStreamDefaultWriter<Uint8Array> | null = null
  private _reader: ReadableStreamDefaultReader<Uint8Array> | null = null
  private _parser = new ChatFrameParser()
  private _heartbeatTimer: ReturnType<typeof setInterval> | null = null
  private _missCount = 0
  private _reconnectAttempts = 0
  private _serverInfo: ServerInfo | null = null
  private _connected = false
  private _destroyed = false
  private _closeNotified = false

  onReady?: () => void
  onMessage?: (reqId: ReqId, payload: string) => void
  onClose?: (reason?: string) => void
  onError?: (err: unknown) => void

  static isSupported(): boolean {
    return getWebTransportConstructor() !== undefined
  }

  connect(info: ServerInfo): void {
    this._serverInfo = info
    this._destroyed = false
    this._closeNotified = false
    this._doConnect()
  }

  send(reqId: ReqId, payload: string): void {
    if (!this._writer) return
    const frame = new Uint8Array(encodeChatFrame(reqId, payload))
    this._writer.write(frame).catch((err: unknown) => {
      this.onError?.(err)
      this._finishClosed("webtransport_write_failed")
    })
  }

  close(): void {
    this._destroyed = true
    this._cleanup()
    try {
      this._transport?.close({ closeCode: 0, reason: "client_close" })
    } catch {
      // Ignore browser implementation close quirks during teardown.
    }
    this._transport = null
  }

  isConnected(): boolean {
    return this._connected
  }

  private _doConnect(): void {
    void this._connectOnce()
  }

  private async _connectOnce(): Promise<void> {
    if (!this._serverInfo || this._destroyed) return

    const TransportCtor = getWebTransportConstructor()
    if (!TransportCtor) {
      this.onError?.(new Error("Browser WebTransport API is unavailable"))
      return
    }
    if (!this._serverInfo.wtUrl) {
      this.onError?.(new Error("WebTransport chat endpoint is missing"))
      return
    }

    this._parser.reset()
    this._connected = false
    this._closeNotified = false

    try {
      const options = this._serverInfo.serverCertificateHashes
        ? { serverCertificateHashes: this._serverInfo.serverCertificateHashes }
        : undefined
      const wt = new TransportCtor(this._serverInfo.wtUrl, options)
      this._transport = wt
      wt.closed.then(
        () => this._finishClosed("webtransport_closed"),
        (err: unknown) => {
          this.onError?.(err)
          this._finishClosed("webtransport_closed")
        },
      )

      await wt.ready
      if (this._destroyed) return

      const stream = await wt.createBidirectionalStream()
      this._writer = stream.writable.getWriter()
      this._reader = stream.readable.getReader()
      void this._readLoop(this._reader)

      this.send(ReqId.ID_CHAT_LOGIN, JSON.stringify({
        protocol_version: this._serverInfo.protocolVersion,
        login_ticket: this._serverInfo.loginTicket,
      }))
    } catch (err) {
      this._cleanup()
      this.onError?.(err)
      this._scheduleReconnect()
    }
  }

  private async _readLoop(reader: ReadableStreamDefaultReader<Uint8Array>): Promise<void> {
    try {
      for (;;) {
        const result = await reader.read()
        if (result.done) {
          this._finishClosed("webtransport_stream_closed")
          return
        }
        const value = result.value
        if (!value) continue

        const bytes = new Uint8Array(value.byteLength)
        bytes.set(value)
        const frames = this._parser.append(bytes.buffer)
        for (const frame of frames) {
          if (frame.reqId === ReqId.ID_HEARTBEAT_RSP) {
            this._missCount = 0
          }
          if (frame.reqId === ReqId.ID_CHAT_LOGIN_RSP) {
            this._connected = true
            this._reconnectAttempts = 0
            this._startHeartbeat()
            this.onReady?.()
          }
          this.onMessage?.(frame.reqId, frame.payload)
        }
      }
    } catch (err) {
      if (!this._destroyed) {
        this.onError?.(err)
        this._finishClosed("webtransport_read_failed")
      }
    }
  }

  private _startHeartbeat(): void {
    this._stopHeartbeat()
    this._missCount = 0
    this._heartbeatTimer = setInterval(() => {
      if (!this._serverInfo) return
      this._missCount++
      if (this._missCount > HEARTBEAT_MAX_MISS) {
        this._transport?.close({ closeCode: 0, reason: "heartbeat_timeout" })
        return
      }
      this.send(ReqId.ID_HEART_BEAT_REQ, JSON.stringify({ fromuid: this._serverInfo.uid }))
    }, HEARTBEAT_INTERVAL_MS)
  }

  private _stopHeartbeat(): void {
    if (this._heartbeatTimer !== null) {
      clearInterval(this._heartbeatTimer)
      this._heartbeatTimer = null
    }
  }

  private _cleanup(): void {
    this._connected = false
    this._stopHeartbeat()
    try {
      this._writer?.releaseLock()
    } catch {
      // The writer may already be closing after transport shutdown.
    }
    this._writer = null
    try {
      this._reader?.releaseLock()
    } catch {
      // Releasing a reader with a pending read can throw during teardown.
    }
    this._reader = null
  }

  private _finishClosed(reason: string): void {
    if (this._closeNotified) return
    this._closeNotified = true
    this._cleanup()
    if (!this._destroyed) {
      this.onClose?.(reason)
      this._scheduleReconnect()
    }
  }

  private _scheduleReconnect(): void {
    if (this._destroyed) return
    const delay = Math.min(
      RECONNECT_BASE_DELAY_MS * Math.pow(1.5, this._reconnectAttempts),
      30_000,
    )
    this._reconnectAttempts++
    setTimeout(() => this._doConnect(), delay)
  }
}
