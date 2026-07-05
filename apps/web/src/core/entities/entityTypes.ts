/** Entity types that back the normalized in-memory cache (mirrors UserMgr) */

export interface Friend {
  uid: number
  name: string
  email: string
  icon: string
  desc?: string
  applyStatus?: number
}

export interface Group {
  groupId: number
  name: string
  icon: string
  memberCount?: number
  announcement?: string
  isMuted?: boolean
  role?: "owner" | "admin" | "member"
}

export interface DialogEntry {
  /** The peer uid (for private) or groupId (for group) */
  peerId: number
  isGroup: boolean
  lastMsgContent: string
  lastMsgTime: number
  unreadCount: number
  isPinned: boolean
  draftText?: string
}

export type MessageState = "sending" | "sent" | "failed" | "received"

export interface RichMessage {
  /** Client-generated id — unique per conversation */
  clientMsgId: string
  /** Server-assigned id (set after ack) */
  serverMsgId?: number
  fromUid: number
  toId: number    // peer uid or groupId
  isGroup: boolean
  content: string
  msgType: "text" | "image" | "file" | "system"
  timestamp: number
  state: MessageState
  /** Set when message is edited */
  editedAt?: number
  /** Set when message is revoked */
  isRevoked?: boolean
}

export interface ApplyEntry {
  fromUid: number
  toUid: number
  applyMsg: string
  status: "pending" | "accepted" | "rejected"
  applyTime: number
}
