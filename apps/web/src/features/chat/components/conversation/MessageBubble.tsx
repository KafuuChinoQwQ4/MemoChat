/** MessageBubble — displays a single chat message */
import type { RichMessage } from "@/core/entities/entityTypes"
import { useEntityStore } from "@/core/entities/entityStore"
import { useSessionStore } from "@/core/session/sessionStore"
import { displayNameWithoutInternalId } from "@/core/entities/displayIds"
import { Avatar } from "@/shared/ui/primitives/Avatar"

export interface MessageBubbleProps {
  message: RichMessage
  showAvatar?: boolean
  stacked?: boolean
}

const AVATAR_SIZE = 34
const AVATAR_SLOT_WIDTH = 40

function senderStatusText(state: RichMessage["state"]): string {
  if (state === "sending") return "发送中"
  if (state === "failed") return "发送失败"
  return ""
}

export function MessageBubble({ message, showAvatar = true, stacked = false }: MessageBubbleProps) {
  const myUid = useSessionStore((s) => s.uid)
  const profile = useSessionStore((s) => s.profile)
  const friendsMap = useEntityStore((s) => s.friends)
  const isSelf = message.fromUid === myUid
  const isRevoked = message.isRevoked === true
  const friend = friendsMap.get(message.fromUid)
  const senderPublicId = message.fromUserId || friend?.userId
  const senderName = isSelf
    ? displayNameWithoutInternalId(profile?.name, profile?.userId, myUid ?? 0, "我")
    : displayNameWithoutInternalId(message.senderName || friend?.name, senderPublicId, message.fromUid, "未知用户")
  const senderIcon = isSelf
    ? (profile?.icon || "")
    : (message.senderIcon || friend?.icon || "")
  const messageAvatarSrc = senderIcon || "head_1"
  const statusText = isSelf ? senderStatusText(message.state) : ""

  const avatarSlot = (
    <div
      style={{
        width: AVATAR_SLOT_WIDTH,
        minWidth: AVATAR_SLOT_WIDTH,
        height: AVATAR_SIZE,
        display: "flex",
        justifyContent: isSelf ? "flex-end" : "flex-start",
        alignItems: "flex-start",
        visibility: showAvatar ? "visible" : "hidden",
      }}
    >
      <Avatar src={messageAvatarSrc} name={senderName} size={AVATAR_SIZE} />
    </div>
  )

  const bubble = (
    <div
      style={{
        maxWidth: isSelf ? "min(56%, 440px)" : "min(64%, 500px)",
        display: "flex",
        flexDirection: "column",
        alignItems: isSelf ? "flex-end" : "flex-start",
      }}
    >
      {/* Sender name — only show for others when visible */}
      {!isSelf && showAvatar && senderName && (
        <div style={{
          maxWidth: "100%",
          margin: "0 0 4px",
          fontSize: 11,
          lineHeight: 1.2,
          color: "var(--text-secondary)",
          overflow: "hidden",
          textOverflow: "ellipsis",
          whiteSpace: "nowrap",
          paddingLeft: 2,
        }}>
          {senderName}
        </div>
      )}

      {/* Bubble */}
      <div style={{
        maxWidth: "100%",
        minHeight: 32,
        padding: "7px 11px",
        borderRadius: isSelf
          ? (stacked ? "10px 4px 10px 10px" : "10px 10px 4px 10px")
          : (stacked ? "4px 10px 10px 10px" : "10px 10px 10px 4px"),
        background: isSelf ? "var(--bubble-self-bg)" : "var(--bubble-other-bg)",
        border: `1px solid ${isSelf ? "var(--bubble-self-border)" : "var(--bubble-other-border)"}`,
        color: isSelf ? "var(--bubble-self-color)" : "var(--bubble-other-color)",
        fontSize: 14,
        lineHeight: 1.5,
        wordBreak: "break-word",
        overflowWrap: "anywhere",
        boxShadow: isSelf ? "var(--bubble-self-shadow)" : "var(--bubble-other-shadow)",
        backdropFilter: "blur(8px) saturate(1.2)",
        WebkitBackdropFilter: "blur(8px) saturate(1.2)",
      }}>
        {isRevoked ? (
          <span style={{ color: "var(--text-disabled)", fontStyle: "italic", fontSize: 13 }}>
            消息已撤回
          </span>
        ) : (
          message.content
        )}
      </div>

      {/* Send status */}
      {statusText ? (
        <div style={{
          marginTop: 3,
          fontSize: 11,
          lineHeight: 1.2,
          color: message.state === "failed" ? "var(--color-badge)" : "var(--text-disabled)",
          textAlign: "right",
        }}>
          {statusText}
        </div>
      ) : null}
    </div>
  )

  return (
    <div style={{
      display: "flex",
      justifyContent: isSelf ? "flex-end" : "flex-start",
      alignItems: "flex-start",
      gap: 8,
      marginTop: stacked ? 3 : 10,
      padding: "0 16px",
    }}>
      {isSelf ? (
        <>
          {bubble}
          {avatarSlot}
        </>
      ) : (
        <>
          {avatarSlot}
          {bubble}
        </>
      )}
    </div>
  )
}
