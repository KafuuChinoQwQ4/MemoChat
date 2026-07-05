/** Session-related types. */

export interface ChatEndpointInfo {
  transport: string
  host: string
  port: string
  path?: string
  tls?: boolean
  serverName?: string
  priority?: number
}

export interface UserProfile {
  uid: number
  name: string
  email: string
  icon: string
  desc?: string
}

/** Full session state stored in sessionStore */
export interface SessionState {
  uid: number | null
  token: string | null
  loginTicket: string | null
  ticketExpireMs: number | null
  refreshToken: string | null
  protocolVersion: number
  chatEndpoints: ChatEndpointInfo[]
  profile: UserProfile | null
  /** WebSocket connection state */
  connState: ConnState
  heartbeatAckMissCount: number
  reconnectAttempts: number
}

export type ConnState =
  | "disconnected"
  | "connecting"
  | "chat_login"
  | "ready"
  | "reconnecting"
