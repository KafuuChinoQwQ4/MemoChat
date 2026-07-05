/** Connection info passed to ChatTransport.connect() */
export interface ServerInfo {
  /** Preferred browser chat transport for this connection attempt. */
  transport: "websocket" | "webtransport"
  /** WebSocket URL, e.g. wss://localhost:8444/ws. Used directly or as WebTransport fallback. */
  wsUrl?: string
  /** WebTransport URL, e.g. https://localhost:8445/chat. */
  wtUrl?: string
  /** Optional browser WebTransport certificate hashes for local self-signed H3. */
  serverCertificateHashes?: Array<{ algorithm: string; value: BufferSource }>
  /** Primary advertised ChatServer name from login chat_endpoints. */
  serverName?: string
  /** Advertised WebSocket ChatServer name, used when falling back. */
  websocketServerName?: string
  /** Advertised WebTransport ChatServer name, used for fallback logs. */
  webtransportServerName?: string
  /** Login ticket for ID_CHAT_LOGIN (1005) */
  loginTicket: string
  /** Bearer token for media/HTTP */
  token: string
  uid: number
  protocolVersion: number
}

/** Transport connection state */
export type ConnState =
  | "disconnected"
  | "connecting"
  | "chat_login"   // waiting for 1006
  | "ready"
  | "reconnecting"
