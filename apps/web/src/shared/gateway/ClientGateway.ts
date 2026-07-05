/**
 * ClientGateway — facade over all infrastructure services.
 * Mirrors the C++ ClientGateway struct.
 * Constructed once in AppProviders, distributed via DI.
 */
import { HttpClient } from "@/core/network/http/HttpClient"
import { BrowserChatTransport } from "@/core/network/transport/BrowserChatTransport"
import { MockChatTransport } from "@/core/network/transport/MockChatTransport"
import { ChatMessageDispatcher } from "@/core/network/dispatcher/ChatMessageDispatcher"
import { SseStreamClient } from "@/core/network/sse/SseStreamClient"
import { runtimeConfig } from "@/core/config/runtimeConfig"
import { useSessionStore } from "@/core/session/sessionStore"
import type { IGateway } from "./gateway.types"

export class ClientGateway implements IGateway {
  readonly http: HttpClient
  readonly chatTransport: ReturnType<typeof buildTransport>
  readonly dispatcher: ChatMessageDispatcher
  readonly sse: SseStreamClient

  constructor() {
    this.dispatcher = new ChatMessageDispatcher()
    this.chatTransport = buildTransport()
    this.http = new HttpClient(
      runtimeConfig.gateBaseUrl,
      () => useSessionStore.getState().token,
    )
    this.sse = new SseStreamClient()
  }

  get session() { return useSessionStore.getState() }
}

function buildTransport() {
  if (runtimeConfig.useMockTransport) return new MockChatTransport()
  return new BrowserChatTransport()
}

/** Singleton — created once when AppProviders mounts */
let _gateway: ClientGateway | null = null

export function getGateway(): ClientGateway {
  if (!_gateway) _gateway = new ClientGateway()
  return _gateway
}

export function resetGateway(): void {
  _gateway?.chatTransport.close()
  _gateway = null
}
