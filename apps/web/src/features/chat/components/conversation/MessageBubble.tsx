/** MessageBubble — displays a single chat message */
import { formatFullTime } from "@/shared/lib/time"
import type { RichMessage } from "@/core/entities/entityTypes"
import { useSessionStore } from "@/core/session/sessionStore"

export interface MessageBubbleProps {
  message: RichMessage
}

export function MessageBubble({ message }: MessageBubbleProps) {
  const myUid = useSessionStore((s) => s.uid)
  const isSelf = message.fromUid === myUid
  const isRevoked = message.isRevoked === true

  return (
    <div style={{
      display: "flex",
      justifyContent: isSelf ? "flex-end" : "flex-start",
      marginBottom: 8,
      padding: "0 12px",
    }}>
      <div style={{
        maxWidth: "65%",
        padding: "8px 12px",
        borderRadius: isSelf ? "16px 16px 4px 16px" : "16px 16px 16px 4px",
        background: isSelf ? "var(--color-brand-green)" : "rgba(255,255,255,0.7)",
        color: isSelf ? "#fff" : "var(--text-primary)",
        fontSize: 14,
        lineHeight: 1.5,
        wordBreak: "break-word",
        position: "relative",
      }}>
        {isRevoked ? (
          <span style={{ color: isSelf ? "rgba(255,255,255,0.7)" : "var(--text-disabled)", fontStyle: "italic", fontSize: 13 }}>
            消息已撤回
          </span>
        ) : (
          message.content
        )}
        <div style={{
          marginTop: 3,
          fontSize: 11,
          color: isSelf ? "rgba(255,255,255,0.65)" : "var(--text-disabled)",
          textAlign: "right",
        }}>
          {formatFullTime(message.timestamp)}
          {isSelf && (
            <span style={{ marginLeft: 4 }}>
              {message.state === "sending" ? "⏳" : message.state === "failed" ? "❌" : "✓"}
            </span>
          )}
        </div>
      </div>
    </div>
  )
}
