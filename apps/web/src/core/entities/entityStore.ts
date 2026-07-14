/**
 * entityStore — normalized Zustand store (mirrors UserMgr in-memory cache).
 * All features access entities through ports, not direct import of this store.
 */
import { create } from "zustand"
import type { Friend, Group, DialogEntry, RichMessage, ApplyEntry, GroupApplyEntry } from "./entityTypes"

interface EntitiesState {
  users: Map<number, { uid: number; name: string; email: string; icon: string }>
  friends: Map<number, Friend>
  groups: Map<number, Group>
  /** keyed by peerId */
  dialogs: Map<number, DialogEntry>
  /** keyed by peerId → array of messages (newest last) */
  messages: Map<number, RichMessage[]>
  applyList: ApplyEntry[]
  /** Pending group join applies for current user as reviewer */
  pendingGroupApplies: GroupApplyEntry[]
  /** IsLoadConFin equivalent — true when all pages loaded */
  dialogListFinished: boolean
}

interface EntitiesActions {
  // Friend actions
  setFriends(list: Friend[]): void
  upsertFriend(f: Friend): void
  removeFriend(uid: number): void

  // Group actions
  setGroups(list: Group[]): void
  upsertGroup(g: Group): void
  removeGroup(id: number): void

  // Dialog actions
  setDialogs(list: DialogEntry[]): void
  upsertDialog(d: DialogEntry): void
  setDialogDraft(peerId: number, draft: string): void
  setDialogPinned(peerId: number, pinned: boolean): void
  clearDialogUnread(peerId: number): void
  removeDialog(peerId: number): void

  // Message actions
  appendMessage(peerId: number, msg: RichMessage): void
  prependMessages(peerId: number, msgs: RichMessage[]): void
  upsertMessage(peerId: number, msg: RichMessage): void
  updateMessageState(peerId: number, clientMsgId: string, state: RichMessage["state"], serverMsgId?: number): void
  patchMessageContent(peerId: number, serverMsgId: number, content: string, editedAt: number): void
  revokeMessage(peerId: number, serverMsgId: number): void

  // Apply list
  setApplyList(list: ApplyEntry[]): void
  appendApply(entry: ApplyEntry): void

  // Group apply list
  setPendingGroupApplies(list: GroupApplyEntry[]): void
  upsertPendingGroupApply(entry: GroupApplyEntry): void
  removePendingGroupApply(applyId: number): void

  // Reset (on logout)
  reset(): void

  // Selectors
  getFriend(uid: number): Friend | undefined
  getGroup(id: number): Group | undefined
  getDialogList(): DialogEntry[]
  getMessages(peerId: number): RichMessage[]
}

const INITIAL: EntitiesState = {
  users: new Map(),
  friends: new Map(),
  groups: new Map(),
  dialogs: new Map(),
  messages: new Map(),
  applyList: [],
  pendingGroupApplies: [],
  dialogListFinished: false,
}

function richMessageKey(msg: RichMessage): string {
  if (msg.serverMsgId !== undefined && msg.serverMsgId > 0) return `server:${msg.isGroup ? "g" : "p"}:${msg.serverMsgId}`
  if (msg.clientMsgId) return `client:${msg.clientMsgId}`
  return `fallback:${msg.fromUid}:${msg.toId}:${msg.timestamp}:${msg.content}`
}

function mergeMessages(existing: RichMessage[], incoming: RichMessage[]): RichMessage[] {
  const byKey = new Map<string, RichMessage>()
  for (const msg of existing) byKey.set(richMessageKey(msg), msg)
  for (const msg of incoming) byKey.set(richMessageKey(msg), msg)
  return Array.from(byKey.values()).sort((a, b) => {
    if (a.timestamp !== b.timestamp) return a.timestamp - b.timestamp
    return a.clientMsgId.localeCompare(b.clientMsgId)
  })
}

export const useEntityStore = create<EntitiesState & EntitiesActions>((set, get) => ({
  ...INITIAL,

  setFriends: (list) => set({ friends: new Map(list.map((f) => [f.uid, f])) }),
  upsertFriend: (f) =>
    set((s) => { const m = new Map(s.friends); m.set(f.uid, f); return { friends: m } }),
  removeFriend: (uid) =>
    set((s) => { const m = new Map(s.friends); m.delete(uid); return { friends: m } }),

  setGroups: (list) => set({ groups: new Map(list.map((g) => [g.groupId, g])) }),
  upsertGroup: (g) =>
    set((s) => { const m = new Map(s.groups); m.set(g.groupId, g); return { groups: m } }),
  removeGroup: (id) =>
    set((s) => { const m = new Map(s.groups); m.delete(id); return { groups: m } }),

  setDialogs: (list) => set({ dialogs: new Map(list.map((d) => [d.peerId, d])) }),
  upsertDialog: (d) =>
    set((s) => { const m = new Map(s.dialogs); m.set(d.peerId, d); return { dialogs: m } }),
  setDialogDraft: (peerId, draft) =>
    set((s) => {
      const m = new Map(s.dialogs)
      const existing = m.get(peerId)
      if (existing) m.set(peerId, { ...existing, draftText: draft })
      return { dialogs: m }
    }),
  setDialogPinned: (peerId, pinned) =>
    set((s) => {
      const m = new Map(s.dialogs)
      const existing = m.get(peerId)
      if (existing) m.set(peerId, { ...existing, isPinned: pinned })
      return { dialogs: m }
    }),
  clearDialogUnread: (peerId) =>
    set((s) => {
      const m = new Map(s.dialogs)
      const existing = m.get(peerId)
      if (existing && existing.unreadCount !== 0) {
        m.set(peerId, { ...existing, unreadCount: 0 })
        return { dialogs: m }
      }
      return {}
    }),
  removeDialog: (peerId) =>
    set((s) => {
      if (!s.dialogs.has(peerId)) return {}
      const m = new Map(s.dialogs)
      m.delete(peerId)
      return { dialogs: m }
    }),

  appendMessage: (peerId, msg) =>
    set((s) => {
      const m = new Map(s.messages)
      m.set(peerId, mergeMessages(m.get(peerId) ?? [], [msg]))
      return { messages: m }
    }),
  prependMessages: (peerId, msgs) =>
    set((s) => {
      const m = new Map(s.messages)
      m.set(peerId, mergeMessages(m.get(peerId) ?? [], msgs))
      return { messages: m }
    }),
  upsertMessage: (peerId, msg) =>
    set((s) => {
      const m = new Map(s.messages)
      m.set(peerId, mergeMessages(m.get(peerId) ?? [], [msg]))
      return { messages: m }
    }),
  updateMessageState: (peerId, clientMsgId, state, serverMsgId) =>
    set((s) => {
      const m = new Map(s.messages)
      const arr = (m.get(peerId) ?? []).map((msg) =>
        msg.clientMsgId === clientMsgId
          ? { ...msg, state, ...(serverMsgId !== undefined ? { serverMsgId } : {}) }
          : msg,
      )
      m.set(peerId, arr)
      return { messages: m }
    }),
  patchMessageContent: (peerId, serverMsgId, content, editedAt) =>
    set((s) => {
      const m = new Map(s.messages)
      const arr = (m.get(peerId) ?? []).map((msg) =>
        msg.serverMsgId === serverMsgId ? { ...msg, content, editedAt } : msg,
      )
      m.set(peerId, arr)
      return { messages: m }
    }),
  revokeMessage: (peerId, serverMsgId) =>
    set((s) => {
      const m = new Map(s.messages)
      const arr = (m.get(peerId) ?? []).map((msg) =>
        msg.serverMsgId === serverMsgId ? { ...msg, isRevoked: true, content: "[消息已撤回]" } : msg,
      )
      m.set(peerId, arr)
      return { messages: m }
    }),

  setApplyList: (list) => set({ applyList: list }),
  appendApply: (entry) => set((s) => ({ applyList: [...s.applyList, entry] })),

  setPendingGroupApplies: (list) => set({ pendingGroupApplies: list }),
  upsertPendingGroupApply: (entry) =>
    set((s) => {
      const others = s.pendingGroupApplies.filter((item) => item.applyId !== entry.applyId)
      return { pendingGroupApplies: [...others, entry] }
    }),
  removePendingGroupApply: (applyId) =>
    set((s) => ({
      pendingGroupApplies: s.pendingGroupApplies.filter((item) => item.applyId !== applyId),
    })),

  reset: () => set({ ...INITIAL }),

  getFriend: (uid) => get().friends.get(uid),
  getGroup: (id) => get().groups.get(id),
  getDialogList: () => {
    const list = Array.from(get().dialogs.values())
    return list.sort((a, b) => {
      if (a.isPinned !== b.isPinned) return a.isPinned ? -1 : 1
      return b.lastMsgTime - a.lastMsgTime
    })
  },
  getMessages: (peerId) => get().messages.get(peerId) ?? [],
}))
