/** runtimeConfig — reads Vite env variables once and exposes typed values */
function envString(name: string, fallback = ""): string {
  const value: unknown = import.meta.env[name]
  return typeof value === "string" ? value : fallback
}

export const runtimeConfig = {
  gateBaseUrl: envString("VITE_GATE_BASE_URL"),
  mediaBaseUrl: envString("VITE_MEDIA_BASE_URL", "https://localhost:8443"),
  aiBaseUrl: envString("VITE_AI_BASE_URL", "https://localhost:8443"),
  wsBridgeUrl: envString("VITE_WS_BRIDGE_URL", "ws://localhost:8444/ws"),
  preferWebTransport: envString("VITE_PREFER_WEBTRANSPORT") !== "0",
  useMockTransport: envString("VITE_USE_MOCK_TRANSPORT") === "1",
  traceHeaders: envString("VITE_TRACE_HEADERS") === "1",
} as const

export type RuntimeConfig = typeof runtimeConfig
