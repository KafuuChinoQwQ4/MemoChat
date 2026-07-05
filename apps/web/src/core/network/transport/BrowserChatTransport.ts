/**
 * BrowserChatTransport — browser transport selector for WebSocket/WebTransport.
 *
 * WebTransport is the preferred browser path when advertised and supported;
 * WebSocket remains the fallback endpoint for browsers or local runtimes that
 * cannot complete WebTransport.
 */
import type { ReqId } from "@/core/network/opcodes/reqIds"
import { logger } from "@/core/common/logger"
import type { ChatTransport } from "./ChatTransport"
import { WebSocketChatTransport } from "./WebSocketChatTransport"
import { WebTransportChatTransport } from "./WebTransportChatTransport"
import type { ServerInfo } from "./transport.types"

export type BrowserChatFallbackReason =
  | "webtransport_unsupported"
  | "webtransport_closed_before_ready"
  | "webtransport_error_before_ready"

export interface BrowserChatTransportDeps {
  createWebSocketTransport?: () => ChatTransport
  createWebTransportTransport?: () => ChatTransport
  isWebTransportSupported?: () => boolean
  logFallback?: (reason: BrowserChatFallbackReason, info: ServerInfo) => void
}

interface ResolvedBrowserChatTransportDeps {
  createWebSocketTransport: () => ChatTransport
  createWebTransportTransport: () => ChatTransport
  isWebTransportSupported: () => boolean
  logFallback: (reason: BrowserChatFallbackReason, info: ServerInfo) => void
}

function defaultFallbackLogger(reason: BrowserChatFallbackReason, info: ServerInfo): void {
  logger.transport.warn("webtransport.fallback_to_websocket", {
    reason,
    uid: info.uid,
    serverName: info.serverName,
    websocketServerName: info.websocketServerName,
    webtransportServerName: info.webtransportServerName,
  })
}

function resolveDeps(deps: BrowserChatTransportDeps): ResolvedBrowserChatTransportDeps {
  return {
    createWebSocketTransport: deps.createWebSocketTransport ?? (() => new WebSocketChatTransport()),
    createWebTransportTransport: deps.createWebTransportTransport ?? (() => new WebTransportChatTransport()),
    isWebTransportSupported: deps.isWebTransportSupported ?? (() => WebTransportChatTransport.isSupported()),
    logFallback: deps.logFallback ?? defaultFallbackLogger,
  }
}

export class BrowserChatTransport implements ChatTransport {
  private readonly _deps: ResolvedBrowserChatTransportDeps
  private _active: ChatTransport | null = null
  private _fallbackStarted = false

  constructor(deps: BrowserChatTransportDeps = {}) {
    this._deps = resolveDeps(deps)
  }

  onReady?: () => void
  onMessage?: (reqId: ReqId, payload: string) => void
  onClose?: (reason?: string) => void
  onError?: (err: unknown) => void

  connect(info: ServerInfo): void {
    this._fallbackStarted = false
    if (info.transport === "webtransport" && info.wtUrl) {
      if (this._deps.isWebTransportSupported()) {
        this._connectWith(this._deps.createWebTransportTransport(), info, true)
        return
      }
      if (info.wsUrl) {
        this._deps.logFallback("webtransport_unsupported", info)
      }
    }

    this._connectWebSocket(info)
  }

  send(reqId: ReqId, payload: string): void {
    this._active?.send(reqId, payload)
  }

  close(): void {
    this._active?.close()
    this._active = null
  }

  isConnected(): boolean {
    return this._active?.isConnected() ?? false
  }

  private _connectWebSocket(info: ServerInfo): void {
    this._connectWith(this._deps.createWebSocketTransport(), { ...info, transport: "websocket" }, false)
  }

  private _connectWith(transport: ChatTransport, info: ServerInfo, allowFallback: boolean): void {
    this._active?.close()
    this._active = transport
    transport.onReady = () => this.onReady?.()
    transport.onMessage = (reqId, payload) => this.onMessage?.(reqId, payload)
    transport.onClose = (reason) => {
      if (
        allowFallback &&
        !transport.isConnected() &&
        this._tryFallback(info, "webtransport_closed_before_ready")
      ) {
        return
      }
      this.onClose?.(reason)
    }
    transport.onError = (err) => {
      if (
        allowFallback &&
        !transport.isConnected() &&
        this._tryFallback(info, "webtransport_error_before_ready")
      ) {
        return
      }
      this.onError?.(err)
    }
    transport.connect(info)
  }

  private _tryFallback(info: ServerInfo, reason: BrowserChatFallbackReason): boolean {
    if (this._fallbackStarted || !info.wsUrl) {
      return false
    }
    this._fallbackStarted = true
    this._deps.logFallback(reason, info)
    const previous = this._active
    this._active = null
    if (previous) {
      delete previous.onClose
      delete previous.onError
      previous.close()
    }
    this._connectWebSocket(info)
    return true
  }
}
