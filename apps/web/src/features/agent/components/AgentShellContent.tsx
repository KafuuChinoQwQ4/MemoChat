/**
 * AgentShellContent — AI chat interface with SSE streaming.
 */
import { useState, useRef, useEffect } from "react"
import { useAgentStore } from "@/features/agent/store/agentStore"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { ENDPOINTS } from "@/core/config/endpoints"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { Spinner } from "@/shared/ui/primitives/Spinner"
import { useSessionStore } from "@/core/session/sessionStore"

interface ChatMessage {
  role: "user" | "assistant"
  content: string
}

function AgentReadyIcon() {
  return (
    <svg
      width="32"
      height="32"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.6"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden
    >
      <rect x="6.2" y="7.2" width="11.6" height="9.8" rx="3" />
      <path d="M9.5 11.2h.1" />
      <path d="M14.4 11.2h.1" />
      <path d="M10 14.1c1.2.8 2.8.8 4 0" />
      <path d="M12 7.2V4.8" />
      <path d="M8.2 19.2h7.6" />
    </svg>
  )
}

export function AgentShellContent() {
  const streaming         = useAgentStore((s) => s.streaming)
  const error             = useAgentStore((s) => s.error)
  const appendStreamChunk = useAgentStore((s) => s.appendStreamChunk)
  const finalizeStream    = useAgentStore((s) => s.finalizeStream)
  const setStreaming       = useAgentStore((s) => s.setStreaming)
  const setError          = useAgentStore((s) => s.setError)
  const token             = useSessionStore((s) => s.token)

  const [messages, setMessages]       = useState<ChatMessage[]>([])
  const [input, setInput]             = useState("")
  const [currentReply, setCurrentReply] = useState("")
  const bottomRef = useRef<HTMLDivElement>(null)
  const replyRef  = useRef("")

  /* Auto-scroll on new content */
  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" })
  }, [messages, currentReply])

  function finishAssistantReply() {
    const reply = replyRef.current
    if (reply) {
      setMessages((prev) => [...prev, { role: "assistant", content: reply }])
    }
    replyRef.current = ""
    setCurrentReply("")
    finalizeStream()
  }

  async function sendMessage() {
    if (!input.trim() || streaming) return
    const userMsg: ChatMessage = { role: "user", content: input.trim() }
    setMessages((prev) => [...prev, userMsg])
    setInput("")
    setCurrentReply("")
    replyRef.current = ""
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
              const next = replyRef.current + chunk.chunk
              replyRef.current = next
              setCurrentReply(next)
              appendStreamChunk(chunk.chunk)
            }
            if (chunk.is_final) finishAssistantReply()
          },
          onError:    (err) => setError(err instanceof Error ? err.message : "流式请求失败"),
          onComplete: () => finishAssistantReply(),
        },
      )
    } catch (err) {
      setError(err instanceof Error ? err.message : "请求失败")
    }
  }

  return (
    <div style={{ height: "100%", display: "flex", flexDirection: "column", position: "relative" }}>
      {/* Message list */}
      <GlassScrollArea style={{ flex: 1, padding: "16px 20px 8px" }}>
        <div style={{ maxWidth: 720, margin: "0 auto" }}>

          {/* Empty state */}
          {messages.length === 0 && !streaming && (
            <div
              style={{
                textAlign: "center",
                color: "var(--text-disabled)",
                fontSize: 14,
                paddingTop: 100,
                animation: "fade-up 300ms cubic-bezier(0.4,0,0.2,1) both",
              }}
            >
              <div
                style={{
                  display: "inline-flex",
                  alignItems: "center",
                  justifyContent: "center",
                  width: 56,
                  height: 56,
                  borderRadius: 16,
                  background: "var(--glass-fill)",
                  backdropFilter: "blur(12px)",
                  border: "1px solid var(--glass-stroke)",
                  marginBottom: 14,
                  color: "var(--text-secondary)",
                }}
              >
                <AgentReadyIcon />
              </div>
              <div style={{ fontWeight: 500, marginBottom: 4, color: "var(--text-secondary)" }}>
                AI 助手已就绪
              </div>
              <div style={{ fontSize: 13 }}>输入消息开始对话</div>
            </div>
          )}

          {/* Message bubbles */}
          {messages.map((msg, i) => (
            <div
              key={i}
              style={{
                marginBottom: 14,
                display: "flex",
                justifyContent: msg.role === "user" ? "flex-end" : "flex-start",
                animation: "fade-up 180ms cubic-bezier(0.4,0,0.2,1) both",
              }}
            >
              <GlassSurface
                style={{
                  maxWidth: "72%",
                  padding: "10px 14px",
                  borderRadius: msg.role === "user"
                    ? "18px 18px 5px 18px"
                    : "18px 18px 18px 5px",
                  background: msg.role === "user"
                    ? "var(--color-brand-green)"
                    : undefined,
                  color: msg.role === "user" ? "#fff" : "var(--text-primary)",
                  fontSize: 14,
                  lineHeight: 1.65,
                  /* User bubble has a richer shadow */
                  boxShadow: msg.role === "user"
                    ? "0 2px 10px rgba(7,193,96,0.30)"
                    : "0 1px 4px rgba(0,0,0,0.07), 0 4px 16px rgba(0,0,0,0.06)",
                }}
              >
                {msg.content}
              </GlassSurface>
            </div>
          ))}

          {/* Streaming reply */}
          {streaming && currentReply && (
            <div
              style={{
                marginBottom: 14,
                display: "flex",
                justifyContent: "flex-start",
                animation: "fade-up 180ms cubic-bezier(0.4,0,0.2,1) both",
              }}
            >
              <GlassSurface
                style={{
                  maxWidth: "72%",
                  padding: "10px 14px",
                  borderRadius: "18px 18px 18px 5px",
                  fontSize: 14,
                  lineHeight: 1.65,
                  boxShadow: "0 1px 4px rgba(0,0,0,0.07), 0 4px 16px rgba(0,0,0,0.06)",
                }}
              >
                {currentReply}
                <span
                  style={{
                    display: "inline-block",
                    animation: "mc-spin 0.9s ease-in-out infinite",
                    marginLeft: 1,
                  }}
                >
                  ▋
                </span>
              </GlassSurface>
            </div>
          )}

          {error && (
            <p
              style={{
                color: "var(--color-badge)",
                fontSize: 13,
                padding: "6px 10px",
                background: "rgba(232,65,65,0.08)",
                borderRadius: 8,
                border: "1px solid rgba(232,65,65,0.18)",
              }}
            >
              {error}
            </p>
          )}
          <div ref={bottomRef} />
        </div>
      </GlassScrollArea>

      {/* Composer */}
      <div
        style={{
          padding: "10px 20px 14px",
          background: "var(--glass-fill)",
          backdropFilter: "blur(16px)",
          WebkitBackdropFilter: "blur(16px)",
          borderTop: "1px solid var(--divider)",
        }}
      >
        <div
          style={{
            maxWidth: 720,
            margin: "0 auto",
            display: "flex",
            gap: 10,
          }}
        >
          <GlassTextField
            value={input}
            onChange={(e) => setInput(e.target.value)}
            placeholder="输入消息… (Enter 发送)"
            onKeyDown={(e) => {
              if (e.key === "Enter" && !e.shiftKey) {
                e.preventDefault()
                void sendMessage()
              }
            }}
            style={{ flex: 1 }}
          />
          <GlassButton
            variant="primary"
            onClick={() => void sendMessage()}
            disabled={streaming || !input.trim()}
          >
            {streaming ? <Spinner size={16} color="#fff" /> : "发送"}
          </GlassButton>
        </div>
      </div>
    </div>
  )
}
