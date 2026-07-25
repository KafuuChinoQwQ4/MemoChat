/** Protocol version constants — keep in sync with global.h */
export const PROTOCOL_VERSION   = 3
export const CLIENT_VERSION     = "3.0.0"
export const CHAT_COUNT_PER_PAGE = 13

/** Chat frame header size: 2 bytes msgId + 2 bytes len */
export const CHAT_FRAME_HEADER_LEN = 4

/** Heartbeat interval (ms) — send ID_HEART_BEAT_REQ every N ms */
export const HEARTBEAT_INTERVAL_MS = 25_000

/** Max missed heartbeat acks before triggering reconnect */
export const HEARTBEAT_MAX_MISS = 2

/** Reconnect backoff: base delay in ms */
export const RECONNECT_BASE_DELAY_MS = 1_500

/** Max uint16 payload length */
export const MAX_FRAME_PAYLOAD = 65_535

/** Group permission bitfield shared by protocol payload readers and writers. */
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

export function normalizeAdminPermissionBits(isAdmin: boolean, permissionBits: number): number {
  if (!isAdmin) return 0
  let normalized = permissionBits > 0 ? permissionBits : GROUP_DEFAULT_ADMIN_PERMISSION_BITS
  normalized &= GROUP_OWNER_PERMISSION_BITS
  return normalized > 0 ? normalized : GROUP_DEFAULT_ADMIN_PERMISSION_BITS
}
