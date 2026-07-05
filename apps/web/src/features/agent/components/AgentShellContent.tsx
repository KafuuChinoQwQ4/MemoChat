/**
 * AgentShellContent — AI chat interface with SSE streaming.
 * Uses SseStreamClient (fetch + ReadableStream) for POST text/event-stream.
 */
import { useState, useRef } from "react"
import { useAgentStore } from "@/features/agent/store/agentStore"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { ENDPOINTS } from "@/core/config/endpoints"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { Spinner } from "@/shared/ui/primitives/Spinner"
import { useSessionStore } from "@/core/session/sessionStore"

interface ChatMessage { role: "user" | "assistant"; content: string }

export function AgentShellContent() {
  const { streaming, appendStreamChunk, finalizeStream, setStreaming, setError, error } = useAgentStore()
  const token = useSessionStore((s) => s.token)
  const [messages, setMessages] = useState<ChatMessage[]>([])
  const [input, setInput] = useState("")
  const [currentReply, setCurrentReply] = useState("")
  const bottomRef = useRef<HTMLDivElement>(null)

  async function sendMessage() {
    if (!input.trim() || streaming) return
    const userMsg: ChatMessage = { role: "user", content: input.trim() }
    setMessages((prev) => [...prev, userMsg])
    setInput("")
    setCurrentReply("")
    setStreaming(true)

    try {
      const sse = getGateway().sse
      await sse.start(
        ENDPOINTS.aiChatStream,
        { messages: [...messages, userMsg].map((m) => ({ role: m.role, content: m.content })) },
        {
          token: token ?? undefined,
          onChunk: (chunk) => {
            if (chunk.chunk) {
              setCurrentReply((prev) => prev + chunk.chunk)
              appendStreamChunk(chunk.chunk)
            }
            if (chunk.is_final) {
              setMessages((prev) => [...prev, { role: "assistant", content: prev.join("") }])
              finalizeStream()
            }
          },
          onError: (err) => setError(err instanceof Error ? err.message : "流式请求失败"),
          onComplete: () => finalizeStream(),
        },
      )
    } catch (err) {
      setError(err instanceof Error ? err.message : "请求失败")
    }
  }

  return (
    <div style={{ height: "100%", display: "flex", flexDirection: "column" }}>
      <GlassScrollArea style={{ flex: 1, padding: "12px 16px" }}>
        <div style={{ maxWidth: 720, margin: "0 auto" }}>
          {messages.length === 0 && !streaming && (
            <div style={{ textAlign: "center", color: "var(--text-disabled)", fontSize: 14, paddingTop: 80 }}>
              🤖 AI助手已就绪，输入消息开始对话
            </div>
          )}
          {messages.map((msg, i) => (
            <div key={i} style={{ marginBottom: 16, display: "flex", justifyContent: msg.role === "user" ? "flex-end" : "flex-start" }}>
              <GlassSurface style={{
                maxWidth: "70%",
                padding: "10px 14px",
                borderRadius: msg.role === "user" ? "16px 16px 4px 16px" : "16px 16px 16px 4px",
                background: msg.role === "user" ? "var(--color-brand-green)" : undefined,
                color: msg.role === "user" ? "#fff" : "var(--text-primary)",
                fontSize: 14,
                lineHeight: 1.6,
              }}>
                {msg.content}
              </GlassSurface>
            </div>
          ))}
          {streaming && currentReply && (
            <div style={{ marginBottom: 16, display: "flex", justifyContent: "flex-start" }}>
              <GlassSurface style={{ maxWidth: "70%", padding: "10px 14px", borderRadius: "16px 16px 16px 4px", fontSize: 14, lineHeight: 1.6 }}>
                {currentReply}<span style={{ animation: "mc-spin 1s linear infinite", display: "inline-block" }}>▋</span>
              </GlassSurface>
            </div>
          )}
          {error && <p style={{ color: "var(--color-badge)", fontSize: 13 }}>{error}</p>}
          <div ref={bottomRef} />
        </div>
      </GlassScrollArea>

      {/* Composer */}
      <div style={{ padding: "8px 16px 12px", borderTop: "1px solid var(--divider)", display: "flex", gap: 8, maxWidth: 720, margin: "0 auto", width: "100%" }}>
        <GlassTextField
          value={input}
          onChange={(e) => setInput(e.target.value)}
          placeholder="输入消息… (Enter 发送)"
          onKeyDown={(e) => { if (e.key === "Enter" && !e.shiftKey) { e.preventDefault(); void sendMessage() } }}
          style={{ flex: 1 }}
        />
        <GlassButton variant="primary" onClick={() => { void sendMessage() }} disabled={streaming || !input.trim()}>
          {streaming ? <Spinner size={18} color="#fff" /> : "发送"}
        </GlassButton>
      </div>
    </div>
  )
}
