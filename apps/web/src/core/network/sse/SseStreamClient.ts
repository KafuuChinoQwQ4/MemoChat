/**
 * SseStreamClient — mirrors AgentStreamClient.
 *
 * AI chat stream is a POST request returning text/event-stream.
 * Cannot use EventSource (no POST support) — use fetch + ReadableStream.
 *
 * Each SSE event's data field is a JSON string: { chunk, is_final, msg_id, ... }
 */
import { parseSseLine } from "./parseSseLine"
import { throwForStatus } from "@/core/network/http/httpErrors"
import { runtimeConfig } from "@/core/config/runtimeConfig"

export interface SseChunk {
  chunk: string
  is_final: boolean
  msg_id?: string
  session_id?: string
  [key: string]: unknown
}

export interface SseStreamOptions {
  onChunk: (chunk: SseChunk) => void
  onError?: (err: unknown) => void
  onComplete?: () => void
  /** Bearer token — pass undefined if not authenticated */
  token?: string | undefined
}

export class SseStreamClient {
  private _controller: AbortController | null = null

  async start(
    path: string,
    body: unknown,
    opts: SseStreamOptions,
  ): Promise<void> {
    this._controller = new AbortController()
    const { signal } = this._controller

    try {
      const headers: Record<string, string> = {
        "Content-Type": "application/json",
      }
      if (opts.token) {
        headers["Authorization"] = `Bearer ${opts.token}`
      }
      const res = await fetch(`${runtimeConfig.aiBaseUrl}${path}`, {
        method: "POST",
        headers,
        body: JSON.stringify(body),
        signal,
      })
      throwForStatus(res)
      if (!res.body) throw new Error("No response body for SSE stream")

      const reader = res.body.getReader()
      const decoder = new TextDecoder()
      let lineBuffer = ""

      while (true) {
        const { done, value } = await reader.read()
        if (done) break
        lineBuffer += decoder.decode(value, { stream: true })
        const lines = lineBuffer.split("\n")
        lineBuffer = lines.pop() ?? ""
        for (const line of lines) {
          const parsed = parseSseLine(line)
          if (parsed?.field === "data") {
            try {
              const chunk = JSON.parse(parsed.value) as SseChunk
              opts.onChunk(chunk)
            } catch {
              // skip malformed lines
            }
          }
        }
      }
      opts.onComplete?.()
    } catch (err) {
      if ((err as Error).name !== "AbortError") {
        opts.onError?.(err)
      }
    }
  }

  cancel(): void {
    this._controller?.abort()
    this._controller = null
  }
}
