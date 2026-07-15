/** DialogListPanel — scrollable list of conversations */
import { useMemo } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { displayNameWithoutInternalId } from "@/core/entities/displayIds"
import { useChatStore } from "@/features/chat/store/chatStore"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { Badge } from "@/shared/ui/primitives/Badge"
import { formatMessageTime } from "@/shared/lib/time"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"

export function DialogListPanel() {
  const dialogsMap = useEntityStore((s) => s.dialogs)
  const friendsMap = useEntityStore((s) => s.friends)
  const groupsMap = useEntityStore((s) => s.groups)
  const dialogs = useMemo(
    () =>
      Array.from(dialogsMap.values()).sort((a, b) => {
        if (a.isPinned !== b.isPinned) return a.isPinned ? -1 : 1
        return b.lastMsgTime - a.lastMsgTime
      }),
    [dialogsMap],
  )
  const selectedPeerId = useChatStore((s) => s.selectedPeerId)

  return (
    <GlassScrollArea style={{ height: "100%", display: "flex", flexDirection: "column" }}>
      {/* Panel header */}
      <div style={{
        padding: "14px 14px 10px",
        fontWeight: 700,
        fontSize: 16,
        color: "var(--text-primary)",
        letterSpacing: "-0.01em",
        flexShrink: 0,
      }}>
        消息
      </div>

      {/* Conversation items */}
      {dialogs.map((d) => {
        const isActive = d.peerId === selectedPeerId
        // Prefer dialog fields, fall back to friend/group entity so a dialog-list
        // row with empty title/avatar still paints after relation bootstrap.
        const friend = !d.isGroup ? friendsMap.get(d.peerId) : undefined
        const group = d.isGroup ? groupsMap.get(d.peerId) : undefined
        const title = d.isGroup
          ? displayNameWithoutInternalId(d.title || group?.name, undefined, d.peerId, "群聊")
          : displayNameWithoutInternalId(d.title || friend?.nick || friend?.name, friend?.userId, d.peerId, "未知用户")
        const avatar = d.avatar || (d.isGroup ? group?.icon : friend?.icon) || ""
        return (
          <button
            key={d.dialogId ?? `${d.isGroup ? "g" : "u"}_${d.peerId}`}
            onClick={() => useChatStore.getState().setSelectedConversation(d.peerId, d.isGroup)}
            style={{
              display: "flex",
              alignItems: "center",
              gap: 10,
              padding: "9px 12px",
              margin: "1px 6px",
              border: "none",
              cursor: "pointer",
              background: isActive ? "var(--tint-selected)" : "transparent",
              textAlign: "left",
              width: "calc(100% - 12px)",
              borderRadius: 10,
              transition: "background 120ms ease",
              flexShrink: 0,
            }}
            onMouseEnter={(e) => {
              if (!isActive) e.currentTarget.style.background = "var(--dialog-item-hover)"
            }}
            onMouseLeave={(e) => {
              if (!isActive) e.currentTarget.style.background = "transparent"
            }}
          >
            <Avatar src={avatar} name={title} size={42} />
            <div style={{ flex: 1, overflow: "hidden" }}>
              <div style={{
                display: "flex",
                justifyContent: "space-between",
                alignItems: "baseline",
                gap: 4,
                marginBottom: 2,
              }}>
                <span style={{
                  fontWeight: 500,
                  fontSize: 14,
                  color: "var(--text-primary)",
                  overflow: "hidden",
                  textOverflow: "ellipsis",
                  whiteSpace: "nowrap",
                  flex: 1,
                }}>
                  {title}
                </span>
                <span style={{
                  fontSize: 11,
                  color: "var(--text-disabled)",
                  flexShrink: 0,
                }}>
                  {formatMessageTime(d.lastMsgTime)}
                </span>
              </div>
              <div style={{
                fontSize: 12,
                color: "var(--text-secondary)",
                overflow: "hidden",
                textOverflow: "ellipsis",
                whiteSpace: "nowrap",
                lineHeight: 1.4,
              }}>
                {d.draftText
                  ? <span style={{ color: "var(--color-badge)" }}>草稿: {d.draftText}</span>
                  : d.lastMsgContent}
              </div>
            </div>
            {d.unreadCount > 0 && <Badge count={d.unreadCount} />}
          </button>
        )
      })}

      {dialogs.length === 0 && (
        <div style={{
          textAlign: "center",
          color: "var(--text-disabled)",
          fontSize: 13,
          padding: "40px 16px",
          lineHeight: 1.6,
        }}>
          暂无消息
        </div>
      )}
    </GlassScrollArea>
  )
}
