import type { HttpClient } from "@/core/network/http/HttpClient"
import type { ChatTransport } from "@/core/network/transport/ChatTransport"
import type { ChatMessageDispatcher } from "@/core/network/dispatcher/ChatMessageDispatcher"
import type { SseStreamClient } from "@/core/network/sse/SseStreamClient"
import type { useSessionStore } from "@/core/session/sessionStore"

type SessionSnapshot = ReturnType<typeof useSessionStore.getState>

/** The gateway facade injected into all feature api/ modules */
export interface IGateway {
  http: HttpClient
  chatTransport: ChatTransport
  dispatcher: ChatMessageDispatcher
  sse: SseStreamClient
  session: Pick<SessionSnapshot, "uid" | "token" | "loginTicket" | "profile" | "isReady" | "setConnState" | "clearSession">
}
