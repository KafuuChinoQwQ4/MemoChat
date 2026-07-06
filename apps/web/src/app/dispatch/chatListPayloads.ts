import { useEntityStore } from "@/core/entities/entityStore"
import type { DialogEntry, Group } from "@/core/entities/entityTypes"

interface GroupListRow {
  groupid?: number | string
  name?: string
  icon?: string
  member_count?: number
  announcement?: string
  role?: number
}

interface GroupListResponse {
  error?: number
  group_list?: GroupListRow[]
}

interface DialogListRow {
  dialog_id?: string
  dialog_type?: string
  peer_uid?: number
  group_id?: number
  title?: string
  avatar?: string
  last_msg_preview?: string
  last_msg_ts?: number
  unread_count?: number
  pinned_rank?: number
  draft_text?: string
}

interface DialogListResponse {
  error?: number
  dialogs?: DialogListRow[]
}

function groupRole(role?: number): "owner" | "admin" | "member" {
  if (role === 3) return "owner"
  if (role === 2) return "admin"
  return "member"
}

export function groupsFromPayload(payload: string): Group[] {
  const data = JSON.parse(payload) as GroupListResponse
  if (data.error !== undefined && data.error !== 0) return []
  return (data.group_list ?? [])
    .map((row): Group | null => {
      const groupId = Number(row.groupid ?? 0)
      if (!Number.isFinite(groupId) || groupId <= 0) return null
      return {
        groupId,
        name: row.name ?? `群组 ${groupId}`,
        icon: row.icon ?? "",
        memberCount: row.member_count ?? 0,
        announcement: row.announcement ?? "",
        role: groupRole(row.role),
      }
    })
    .filter((group): group is Group => group !== null)
}

export function dialogsFromPayload(payload: string): DialogEntry[] {
  const data = JSON.parse(payload) as DialogListResponse
  if (data.error !== undefined && data.error !== 0) return []
  return (data.dialogs ?? [])
    .map((row): DialogEntry | null => {
      const isGroup = row.dialog_type === "group"
      const peerId = isGroup ? Number(row.group_id ?? 0) : Number(row.peer_uid ?? 0)
      if (!Number.isFinite(peerId) || peerId <= 0) return null
      return {
        ...(row.dialog_id ? { dialogId: row.dialog_id } : {}),
        peerId,
        isGroup,
        ...(row.title ? { title: row.title } : {}),
        ...(row.avatar ? { avatar: row.avatar } : {}),
        lastMsgContent: row.last_msg_preview ?? "",
        lastMsgTime: row.last_msg_ts ?? 0,
        unreadCount: row.unread_count ?? 0,
        isPinned: (row.pinned_rank ?? 0) > 0,
        ...(row.draft_text ? { draftText: row.draft_text } : {}),
      }
    })
    .filter((dialog): dialog is DialogEntry => dialog !== null)
}

export function applyGroupListPayload(payload: string): void {
  useEntityStore.getState().setGroups(groupsFromPayload(payload))
}

export function applyDialogListPayload(payload: string): void {
  useEntityStore.getState().setDialogs(dialogsFromPayload(payload))
}
