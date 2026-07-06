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
      {!isSelf && showAvatar && senderName && (
        <div style={{
          maxWidth: "100%",
          margin: "0 0 4px",
          fontSize: 11,
          lineHeight: 1.2,
          color: "rgba(67, 85, 108, 0.74)",
          overflow: "hidden",
          textOverflow: "ellipsis",
          whiteSpace: "nowrap",
        }}>
          {senderName}
        </div>
      )}
      <div style={{
        maxWidth: "100%",
        minHeight: 30,
        padding: "5px 8px",
        borderRadius: isSelf
          ? (stacked ? "8px 3px 8px 8px" : "8px 8px 3px 8px")
          : (stacked ? "3px 8px 8px 8px" : "8px 8px 8px 3px"),
        background: isSelf ? "rgba(177, 217, 255, 0.82)" : "rgba(255, 255, 255, 0.74)",
        border: isSelf ? "1px solid rgba(101, 165, 233, 0.82)" : "1px solid rgba(255, 255, 255, 0.76)",
        color: isSelf ? "rgba(24, 50, 76, 0.94)" : "rgba(24, 31, 42, 0.94)",
        fontSize: 14,
        lineHeight: 1.38,
        wordBreak: "break-word",
        overflowWrap: "anywhere",
        boxShadow: isSelf
          ? "0 4px 10px rgba(64, 130, 205, 0.12)"
          : "0 5px 14px rgba(31, 45, 63, 0.07)",
      }}>
        {isRevoked ? (
          <span style={{ color: "var(--text-disabled)", fontStyle: "italic", fontSize: 13 }}>
            消息已撤回
          </span>
        ) : (
          message.content
        )}
      </div>
      {statusText ? (
        <div style={{
          marginTop: 4,
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
      gap: 7,
      marginTop: stacked ? 3 : 8,
      padding: "0 14px",
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
