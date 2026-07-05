/** DialogListPanel — scrollable list of conversations */
import { useMemo } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { useChatStore } from "@/features/chat/store/chatStore"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { Badge } from "@/shared/ui/primitives/Badge"
import { formatMessageTime } from "@/shared/lib/time"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"

export function DialogListPanel() {
  // Subscribe to the dialogs Map (stable reference), sort in useMemo.
  // Never call getDialogList() directly in a selector — it returns a new
  // array every call, causing Zustand to force an infinite re-render loop.
  const dialogsMap = useEntityStore((s) => s.dialogs)
  const dialogs = useMemo(
    () =>
      Array.from(dialogsMap.values()).sort((a, b) => {
        if (a.isPinned !== b.isPinned) return a.isPinned ? -1 : 1
        return b.lastMsgTime - a.lastMsgTime
      }),
    [dialogsMap],
  )
  const { selectedPeerId, setSelectedConversation } = useChatStore()

  return (
    <GlassScrollArea style={{ height: "100%", display: "flex", flexDirection: "column" }}>
      <div style={{ padding: "12px 12px 8px", fontWeight: 600, fontSize: 15 }}>消息</div>
      {dialogs.map((d) => {
        const isActive = d.peerId === selectedPeerId
        return (
          <button
            key={d.peerId}
            onClick={() => setSelectedConversation(d.peerId, d.isGroup)}
            style={{
              display: "flex",
              alignItems: "center",
              gap: 10,
              padding: "8px 12px",
              border: "none",
              cursor: "pointer",
              background: isActive ? "var(--tint-selected)" : "transparent",
              textAlign: "left",
              width: "100%",
              borderRadius: 8,
              transition: "background 100ms ease",
            }}
          >
            <Avatar src={undefined} name={d.peerId.toString()} size={40} />
            <div style={{ flex: 1, overflow: "hidden" }}>
              <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
                <span style={{ fontWeight: 500, fontSize: 14, color: "var(--text-primary)" }}>{d.peerId}</span>
                <span style={{ fontSize: 11, color: "var(--text-disabled)" }}>{formatMessageTime(d.lastMsgTime)}</span>
              </div>
              <div style={{ fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                {d.draftText ? <span style={{ color: "var(--color-badge)" }}>草稿: {d.draftText}</span> : d.lastMsgContent}
              </div>
            </div>
            {d.unreadCount > 0 && <Badge count={d.unreadCount} />}
          </button>
        )
      })}
      {dialogs.length === 0 && (
        <div style={{ textAlign: "center", color: "var(--text-disabled)", fontSize: 13, padding: 32 }}>
          暂无消息
        </div>
      )}
    </GlassScrollArea>
  )
}
