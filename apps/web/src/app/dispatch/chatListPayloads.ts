import { useEntityStore } from "@/core/entities/entityStore"
import { normalizeGroupPublicId } from "@/core/entities/displayIds"
import type { DialogEntry, Group, GroupApplyEntry } from "@/core/entities/entityTypes"

/** Mirrors desktop GroupRequestPayloads permission bits */
export const GROUP_PERM_CHANGE_INFO = 1 << 0
export const GROUP_PERM_DELETE_MESSAGES = 1 << 1
export const GROUP_PERM_INVITE_USERS = 1 << 2
export const GROUP_PERM_MANAGE_ADMINS = 1 << 3
export const GROUP_PERM_PIN_MESSAGES = 1 << 4
export const GROUP_PERM_BAN_USERS = 1 << 5
export const GROUP_PERM_MANAGE_TOPICS = 1 << 6
export const GROUP_OWNER_PERMISSION_BITS =
  GROUP_PERM_CHANGE_INFO |
  GROUP_PERM_DELETE_MESSAGES |
  GROUP_PERM_INVITE_USERS |
  GROUP_PERM_MANAGE_ADMINS |
  GROUP_PERM_PIN_MESSAGES |
  GROUP_PERM_BAN_USERS |
  GROUP_PERM_MANAGE_TOPICS
export const GROUP_DEFAULT_ADMIN_PERMISSION_BITS =
  GROUP_PERM_CHANGE_INFO |
  GROUP_PERM_DELETE_MESSAGES |
  GROUP_PERM_INVITE_USERS |
  GROUP_PERM_PIN_MESSAGES |
  GROUP_PERM_BAN_USERS

interface GroupListRow {
  groupid?: number | string
  group_code?: string
  name?: string
  icon?: string
  member_count?: number
  announcement?: string
  role?: number
  permission_bits?: number | string
  can_change_group_info?: boolean
  can_delete_messages?: boolean
  can_invite_users?: boolean
  can_manage_admins?: boolean
  can_pin_messages?: boolean
  can_ban_users?: boolean
  can_manage_topics?: boolean
}


interface PendingGroupApplyRow {
  apply_id?: number | string
  groupid?: number | string
  group_id?: number | string
  group_code?: string
  applicant_uid?: number | string
  applicant_user_id?: string
  inviter_uid?: number | string
  inviter_user_id?: string
  type?: number | string
  status?: number | string
  reason?: string
}

interface GroupListResponse {
  error?: number
  group_list?: GroupListRow[]
  pending_group_apply_list?: PendingGroupApplyRow[]
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

export function groupRole(role?: number): "owner" | "admin" | "member" {
  if (role === 3) return "owner"
  if (role === 2) return "admin"
  return "member"
}

export function groupRoleRank(role?: Group["role"]): number {
  if (role === "owner") return 3
  if (role === "admin") return 2
  return 1
}

function asNumber(value: unknown): number | undefined {
  if (typeof value === "number" && Number.isFinite(value)) return value
  if (typeof value === "string" && value.trim()) {
    const n = Number(value)
    if (Number.isFinite(n)) return n
  }
  return undefined
}

export function deriveGroupPermissions(
  role: Group["role"] | undefined,
  permissionBitsRaw: number | undefined,
  flags?: {
    can_change_group_info?: boolean
    can_delete_messages?: boolean
    can_invite_users?: boolean
    can_manage_admins?: boolean
    can_pin_messages?: boolean
    can_ban_users?: boolean
    can_manage_topics?: boolean
  },
): Pick<
  Group,
  | "permissionBits"
  | "canChangeInfo"
  | "canDeleteMessages"
  | "canInviteUsers"
  | "canManageAdmins"
  | "canPinMessages"
  | "canBanUsers"
  | "canManageTopics"
> {
  let bits = permissionBitsRaw ?? 0
  if ((!bits || bits <= 0) && role === "owner") {
    bits = GROUP_OWNER_PERMISSION_BITS
  } else if ((!bits || bits <= 0) && role === "admin") {
    // Prefer explicit can_* flags when bits missing.
    if (flags) {
      bits = 0
      if (flags.can_change_group_info) bits |= GROUP_PERM_CHANGE_INFO
      if (flags.can_delete_messages) bits |= GROUP_PERM_DELETE_MESSAGES
      if (flags.can_invite_users) bits |= GROUP_PERM_INVITE_USERS
      if (flags.can_manage_admins) bits |= GROUP_PERM_MANAGE_ADMINS
      if (flags.can_pin_messages) bits |= GROUP_PERM_PIN_MESSAGES
      if (flags.can_ban_users) bits |= GROUP_PERM_BAN_USERS
      if (flags.can_manage_topics) bits |= GROUP_PERM_MANAGE_TOPICS
      if (bits <= 0) bits = GROUP_DEFAULT_ADMIN_PERMISSION_BITS
    } else {
      bits = GROUP_DEFAULT_ADMIN_PERMISSION_BITS
    }
  }

  const has = (mask: number, flag?: boolean) => {
    if (typeof flag === "boolean") return flag
    return (bits & mask) !== 0
  }

  return {
    permissionBits: bits,
    canChangeInfo: has(GROUP_PERM_CHANGE_INFO, flags?.can_change_group_info),
    canDeleteMessages: has(GROUP_PERM_DELETE_MESSAGES, flags?.can_delete_messages),
    canInviteUsers: has(GROUP_PERM_INVITE_USERS, flags?.can_invite_users),
    canManageAdmins: has(GROUP_PERM_MANAGE_ADMINS, flags?.can_manage_admins),
    canPinMessages: has(GROUP_PERM_PIN_MESSAGES, flags?.can_pin_messages),
    canBanUsers: has(GROUP_PERM_BAN_USERS, flags?.can_ban_users),
    canManageTopics: has(GROUP_PERM_MANAGE_TOPICS, flags?.can_manage_topics),
  }
}

export function groupsFromPayload(payload: string): Group[] {
  const data = JSON.parse(payload) as GroupListResponse
  if (data.error !== undefined && data.error !== 0) return []
  return (data.group_list ?? [])
    .map((row): Group | null => {
      const groupId = Number(row.groupid ?? 0)
      if (!Number.isFinite(groupId) || groupId <= 0) return null
      const groupCode = normalizeGroupPublicId(row.group_code)
      const role = groupRole(row.role)
      const perms = deriveGroupPermissions(role, asNumber(row.permission_bits), row)
      return {
        groupId,
        ...(groupCode ? { groupCode } : {}),
        name: row.name ?? (groupCode ? `群组 ${groupCode}` : "未命名群聊"),
        icon: row.icon ?? "",
        memberCount: row.member_count ?? 0,
        announcement: row.announcement ?? "",
        role,
        ...perms,
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


export function pendingGroupAppliesFromPayload(payload: string): GroupApplyEntry[] {
  const data = JSON.parse(payload) as GroupListResponse
  if (data.error !== undefined && data.error !== 0) return []
  return (data.pending_group_apply_list ?? [])
    .map((row): GroupApplyEntry | null => {
      const applyId = Number(row.apply_id ?? 0)
      const groupId = Number(row.groupid ?? row.group_id ?? 0)
      const applicantUid = Number(row.applicant_uid ?? 0)
      if (!Number.isFinite(applyId) || applyId <= 0) return null
      if (!Number.isFinite(groupId) || groupId <= 0) return null
      const groupCode = normalizeGroupPublicId(row.group_code)
      const applicantUserId = typeof row.applicant_user_id === "string" ? row.applicant_user_id.trim() : ""
      const inviterUid = asNumber(row.inviter_uid)
      const inviterUserId = typeof row.inviter_user_id === "string" ? row.inviter_user_id.trim() : ""
      const type = asNumber(row.type)
      const status = asNumber(row.status)
      const reason = typeof row.reason === "string" ? row.reason.trim() : ""
      return {
        applyId,
        groupId,
        ...(groupCode ? { groupCode } : {}),
        applicantUid: Number.isFinite(applicantUid) ? applicantUid : 0,
        ...(applicantUserId ? { applicantUserId } : {}),
        ...(inviterUid && inviterUid > 0 ? { inviterUid } : {}),
        ...(inviterUserId ? { inviterUserId } : {}),
        ...(type !== undefined ? { type } : {}),
        ...(status !== undefined ? { status } : {}),
        ...(reason ? { reason } : {}),
      }
    })
    .filter((entry): entry is GroupApplyEntry => entry !== null)
}

export function applyGroupListPayload(payload: string): void {
  const store = useEntityStore.getState()
  store.setGroups(groupsFromPayload(payload))
  // Only overwrite pending applies when the payload includes the field.
  try {
    const data = JSON.parse(payload) as GroupListResponse
    if (Array.isArray(data.pending_group_apply_list)) {
      store.setPendingGroupApplies(pendingGroupAppliesFromPayload(payload))
    }
  } catch {
    // ignore parse errors; groups already applied above when possible
  }
}

export function applyDialogListPayload(payload: string): void {
  useEntityStore.getState().setDialogs(dialogsFromPayload(payload))
}

export function normalizeAdminPermissionBits(isAdmin: boolean, permissionBits: number): number {
  if (!isAdmin) return 0
  let normalized = permissionBits > 0 ? permissionBits : GROUP_DEFAULT_ADMIN_PERMISSION_BITS
  normalized &= GROUP_OWNER_PERMISSION_BITS
  return normalized > 0 ? normalized : GROUP_DEFAULT_ADMIN_PERMISSION_BITS
}
