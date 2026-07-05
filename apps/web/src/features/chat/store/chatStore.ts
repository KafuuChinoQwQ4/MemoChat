/**
 * chatStore — UI state for the chat feature.
 * Entity data (messages, dialogs) lives in entityStore;
 * this store only holds: selected dialog, composer text, pagination state.
 */
import { create } from "zustand"

interface ChatStore {
  /** Currently selected conversation peer id (uid or groupId) */
  selectedPeerId: number | null
  selectedIsGroup: boolean
  composerText: string
  loadingHistory: boolean
  historyFinished: boolean

  setSelectedConversation(peerId: number, isGroup: boolean): void
  setComposerText(text: string): void
  setLoadingHistory(v: boolean): void
  setHistoryFinished(v: boolean): void
  reset(): void
}

export const useChatStore = create<ChatStore>((set) => ({
  selectedPeerId: null,
  selectedIsGroup: false,
  composerText: "",
  loadingHistory: false,
  historyFinished: false,

  setSelectedConversation: (peerId, isGroup) =>
    set({ selectedPeerId: peerId, selectedIsGroup: isGroup, composerText: "", historyFinished: false }),
  setComposerText: (text) => set({ composerText: text }),
  setLoadingHistory: (v) => set({ loadingHistory: v }),
  setHistoryFinished: (v) => set({ historyFinished: v }),
  reset: () => set({ selectedPeerId: null, selectedIsGroup: false, composerText: "", loadingHistory: false, historyFinished: false }),
}))
