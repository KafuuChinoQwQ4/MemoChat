/** runtimeConfig — reads Vite env variables once and exposes typed values */
export const runtimeConfig = {
  gateBaseUrl: import.meta.env["VITE_GATE_BASE_URL"] ?? "https://localhost:8443",
  mediaBaseUrl: import.meta.env["VITE_MEDIA_BASE_URL"] ?? "https://localhost:8443",
  aiBaseUrl: import.meta.env["VITE_AI_BASE_URL"] ?? "https://localhost:8443",
  wsBridgeUrl: import.meta.env["VITE_WS_BRIDGE_URL"] ?? "wss://localhost:8444/ws",
  preferWebTransport: import.meta.env["VITE_PREFER_WEBTRANSPORT"] === "1",
  useMockTransport: import.meta.env["VITE_USE_MOCK_TRANSPORT"] === "1",
  traceHeaders: import.meta.env["VITE_TRACE_HEADERS"] === "1",
} as const

export type RuntimeConfig = typeof runtimeConfig
