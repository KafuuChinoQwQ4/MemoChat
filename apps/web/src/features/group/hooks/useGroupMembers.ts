/** useGroupMembers — assemble displayable group members from local caches */
import { useMemo } from "react"
import type { Friend, Group, GroupMemberEntry, RichMessage } from "@/core/entities/entityTypes"
import { displayNameWithoutInternalId, normalizePublicUserId } from "@/core/entities/displayIds"
import { useSessionStore } from "@/core/session/sessionStore"

function withOptionalUserId(userId: string | undefined): Pick<GroupMemberEntry, "userId"> {
  return userId ? { userId } : {}
}

function withOptionalIcon(icon: string | undefined): Pick<GroupMemberEntry, "icon"> {
  return icon ? { icon } : {}
}

function withOptionalRole(role: GroupMemberEntry["role"] | undefined): Pick<GroupMemberEntry, "role"> {
  return role ? { role } : {}
}

export function useGroupMembers(
  group: Group | null | undefined,
  friends: Friend[],
  messages: RichMessage[],
): GroupMemberEntry[] {
  const uid = useSessionStore((s) => s.uid) ?? 0
  const profile = useSessionStore((s) => s.profile)

  return useMemo(() => {
    if (!group) return []
    const byUid = new Map<number, GroupMemberEntry>()

    const upsert = (entry: GroupMemberEntry) => {
      if (!entry.uid || entry.uid <= 0) return
      const existing = byUid.get(entry.uid)
      if (!existing) {
        byUid.set(entry.uid, entry)
        return
      }
      byUid.set(entry.uid, {
        ...existing,
        ...entry,
        ...withOptionalUserId(entry.userId || existing.userId),
        name: entry.name || existing.name,
        ...withOptionalIcon(entry.icon || existing.icon),
        ...withOptionalRole(entry.role || existing.role),
        source: existing.source === "self" || existing.source === "friend" ? existing.source : entry.source,
      })
    }

    // Self
    if (uid > 0) {
      upsert({
        uid,
        ...withOptionalUserId(normalizePublicUserId(profile?.userId) || undefined),
        name: displayNameWithoutInternalId(profile?.name, profile?.userId, uid, "我"),
        ...withOptionalIcon(profile?.icon || ""),
        ...withOptionalRole(group.role),
        source: "self",
      })
    }

    // Speakers seen in this group conversation.
    // Protocol currently does not push a full roster; enrich names/icons from friends when available.
    const friendByUid = new Map(friends.map((friend) => [friend.uid, friend]))
    for (const msg of messages) {
      if (!msg.isGroup || msg.toId !== group.groupId) continue
      if (!msg.fromUid || msg.fromUid <= 0) continue
      const friend = friendByUid.get(msg.fromUid)
      upsert({
        uid: msg.fromUid,
        ...withOptionalUserId(normalizePublicUserId(msg.fromUserId || friend?.userId) || undefined),
        name: displayNameWithoutInternalId(
          msg.senderName || friend?.nick || friend?.name,
          msg.fromUserId || friend?.userId,
          msg.fromUid,
          "群成员",
        ),
        ...withOptionalIcon(msg.senderIcon || friend?.icon || ""),
        source: friend ? "friend" : "message",
      })
    }

    return Array.from(byUid.values())
  }, [friends, group, messages, profile?.icon, profile?.name, profile?.userId, uid])
}
