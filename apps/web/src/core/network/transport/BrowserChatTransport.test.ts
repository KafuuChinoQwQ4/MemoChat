import { afterEach, describe, expect, it, vi } from "vitest"
import type { ReqId } from "@/core/network/opcodes/reqIds"
import type { ChatTransport } from "./ChatTransport"
import {
  BrowserChatTransport,
  type BrowserChatFallbackReason,
} from "./BrowserChatTransport"
import type { ServerInfo } from "./transport.types"

class FakeTransport implements ChatTransport {
  readonly connectCalls: ServerInfo[] = []
  readonly sendCalls: Array<{ reqId: ReqId; payload: string }> = []
  closeCalls = 0
  connected = false

  onReady?: () => void
  onMessage?: (reqId: ReqId, payload: string) => void
  onClose?: (reason?: string) => void
  onError?: (err: unknown) => void

  connect(info: ServerInfo): void {
    this.connectCalls.push(info)
  }

  send(reqId: ReqId, payload: string): void {
    this.sendCalls.push({ reqId, payload })
  }

  close(): void {
    this.closeCalls += 1
  }

  isConnected(): boolean {
    return this.connected
  }

  triggerError(err: unknown): void {
    this.onError?.(err)
  }

  triggerClose(reason?: string): void {
    this.onClose?.(reason)
  }
}

function serverInfo(overrides: Partial<ServerInfo> = {}): ServerInfo {
  return {
    transport: "websocket",
    wsUrl: "ws://localhost:8444/ws",
    loginTicket: "ticket",
    token: "token",
    uid: 42,
    protocolVersion: 3,
    serverName: "chatserver1",
    websocketServerName: "chatserver1",
    ...overrides,
  }
}

describe("BrowserChatTransport", () => {
  afterEach(() => {
    vi.useRealTimers()
  })

  it("uses websocket transport for the default browser path", () => {
    const websocket = new FakeTransport()
    const webtransport = new FakeTransport()
    const selector = new BrowserChatTransport({
      createWebSocketTransport: () => websocket,
      createWebTransportTransport: () => webtransport,
      isWebTransportSupported: () => true,
    })

    selector.connect(serverInfo())

    expect(websocket.connectCalls).toHaveLength(1)
    expect(websocket.connectCalls[0]?.transport).toBe("websocket")
    expect(webtransport.connectCalls).toHaveLength(0)
  })

  it("uses webtransport only when requested and browser support is present", () => {
    const websocket = new FakeTransport()
    const webtransport = new FakeTransport()
    const selector = new BrowserChatTransport({
      createWebSocketTransport: () => websocket,
      createWebTransportTransport: () => webtransport,
      isWebTransportSupported: () => true,
    })

    selector.connect(serverInfo({
      transport: "webtransport",
      wtUrl: "https://localhost:8445/chat",
      webtransportServerName: "chatserver1",
    }))

    expect(webtransport.connectCalls).toHaveLength(1)
    expect(webtransport.connectCalls[0]?.transport).toBe("webtransport")
    expect(websocket.connectCalls).toHaveLength(0)
  })

  it("falls back to websocket and logs when webtransport is unsupported", () => {
    const websocket = new FakeTransport()
    const logs: BrowserChatFallbackReason[] = []
    const selector = new BrowserChatTransport({
      createWebSocketTransport: () => websocket,
      isWebTransportSupported: () => false,
      logFallback: (reason) => logs.push(reason),
    })

    selector.connect(serverInfo({
      transport: "webtransport",
      wtUrl: "https://localhost:8445/chat",
      webtransportServerName: "chatserver1",
    }))

    expect(websocket.connectCalls).toHaveLength(1)
    expect(websocket.connectCalls[0]?.transport).toBe("websocket")
    expect(logs).toEqual(["webtransport_unsupported"])
  })

  it("falls back to websocket when webtransport fails before ready", () => {
    const websocket = new FakeTransport()
    const webtransport = new FakeTransport()
    const logs: BrowserChatFallbackReason[] = []
    const selector = new BrowserChatTransport({
      createWebSocketTransport: () => websocket,
      createWebTransportTransport: () => webtransport,
      isWebTransportSupported: () => true,
      logFallback: (reason) => logs.push(reason),
    })

    selector.connect(serverInfo({
      transport: "webtransport",
      wtUrl: "https://localhost:8445/chat",
      webtransportServerName: "chatserver1",
    }))
    webtransport.triggerError(new Error("h3 failed"))

    expect(webtransport.closeCalls).toBe(1)
    expect(websocket.connectCalls).toHaveLength(1)
    expect(websocket.connectCalls[0]?.transport).toBe("websocket")
    expect(logs).toEqual(["webtransport_error_before_ready"])
  })

  it("falls back to websocket when webtransport stays pending before ready", () => {
    vi.useFakeTimers()
    const websocket = new FakeTransport()
    const webtransport = new FakeTransport()
    const logs: BrowserChatFallbackReason[] = []
    const selector = new BrowserChatTransport({
      createWebSocketTransport: () => websocket,
      createWebTransportTransport: () => webtransport,
      isWebTransportSupported: () => true,
      logFallback: (reason) => logs.push(reason),
      webTransportFallbackTimeoutMs: 25,
    })

    selector.connect(serverInfo({
      transport: "webtransport",
      wtUrl: "https://localhost:8445/chat",
      webtransportServerName: "chatserver1",
    }))
    vi.advanceTimersByTime(24)
    expect(websocket.connectCalls).toHaveLength(0)

    vi.advanceTimersByTime(1)

    expect(webtransport.closeCalls).toBe(1)
    expect(websocket.connectCalls).toHaveLength(1)
    expect(websocket.connectCalls[0]?.transport).toBe("websocket")
    expect(logs).toEqual(["webtransport_ready_timeout"])
  })
})
