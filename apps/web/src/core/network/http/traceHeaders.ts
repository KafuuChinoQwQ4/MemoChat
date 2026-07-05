import { runtimeConfig } from "@/core/config/runtimeConfig"

let _seq = 0

/** Build optional trace headers for fetch requests */
export function buildTraceHeaders(): Record<string, string> {
  if (!runtimeConfig.traceHeaders) return {}
  return {
    "X-Request-ID": `web-${Date.now()}-${_seq++}`,
    "X-Client-Version": "3.0.0",
  }
}
