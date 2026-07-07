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
      padding: "10px 14px",
      borderTop: "1px solid var(--composer-border)",
      background: "var(--composer-bg)",
      backdropFilter: "blur(16px) saturate(1.4)",
      WebkitBackdropFilter: "blur(16px) saturate(1.4)",
      alignItems: "flex-end",
    }}>
      <textarea
        ref={textareaRef}
        value={composerText}
        onChange={(e) => setComposerText(e.target.value)}
        onKeyDown={handleKeyDown}
        onFocus={(e) => {
          e.currentTarget.style.borderColor = "var(--composer-input-focus-border)"
          e.currentTarget.style.boxShadow = "var(--composer-input-focus-shadow)"
        }}
        onBlur={(e) => {
          e.currentTarget.style.borderColor = "var(--composer-input-border)"
          e.currentTarget.style.boxShadow = "none"
        }}
        placeholder="输入消息… (Enter 发送，Shift+Enter 换行)"
        rows={1}
        style={{
          flex: 1,
          resize: "none",
          border: "1px solid var(--composer-input-border)",
          borderRadius: 10,
          padding: "9px 13px",
          fontSize: 14,
          background: "var(--composer-input-bg)",
          color: "var(--text-primary)",
          outline: "none",
          minHeight: 38,
          maxHeight: 120,
          overflowY: "auto",
          lineHeight: 1.5,
          fontFamily: "var(--font-family-zh)",
          transition: "border-color 150ms ease, box-shadow 150ms ease",
        }}
      />
      <GlassButton
        variant="primary"
        onClick={submit}
        disabled={!composerText.trim()}
        style={{ padding: "9px 20px", flexShrink: 0 }}
      >
        发送
      </GlassButton>
    </div>
  )
}
