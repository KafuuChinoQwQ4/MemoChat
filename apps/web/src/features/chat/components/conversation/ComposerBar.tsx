/** ComposerBar — text input + send button for chat messages */
import { useRef } from "react"
import { useChatStore } from "@/features/chat/store/chatStore"
import { GlassButton } from "@/shared/ui/glass/GlassButton"

interface ComposerBarProps {
  onSend(text: string): void
}

export function ComposerBar({ onSend }: ComposerBarProps) {
  const { composerText, setComposerText } = useChatStore()
  const textareaRef = useRef<HTMLTextAreaElement>(null)

  function handleKeyDown(e: React.KeyboardEvent) {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault()
      submit()
    }
  }

  function submit() {
    const text = composerText.trim()
    if (!text) return
    onSend(text)
    setComposerText("")
    textareaRef.current?.focus()
  }

  return (
    <div style={{
      display: "flex",
      gap: 8,
      padding: "8px 12px",
      borderTop: "1px solid var(--divider)",
      background: "rgba(255,255,255,0.05)",
      alignItems: "flex-end",
    }}>
      <textarea
        ref={textareaRef}
        value={composerText}
        onChange={(e) => setComposerText(e.target.value)}
        onKeyDown={handleKeyDown}
        placeholder="输入消息… (Enter 发送，Shift+Enter 换行)"
        rows={1}
        style={{
          flex: 1,
          resize: "none",
          border: "1px solid rgba(0,0,0,0.1)",
          borderRadius: 8,
          padding: "8px 12px",
          fontSize: 14,
          background: "rgba(255,255,255,0.6)",
          outline: "none",
          minHeight: 36,
          maxHeight: 120,
          overflowY: "auto",
          lineHeight: 1.5,
          fontFamily: "var(--font-family-zh)",
        }}
      />
      <GlassButton
        variant="primary"
        onClick={submit}
        disabled={!composerText.trim()}
        style={{ padding: "8px 18px", flexShrink: 0 }}
      >
        发送
      </GlassButton>
    </div>
  )
}
