/**
 * chatStore — UI state for the chat feature.
 * Entity data (messages, dialogs) lives in entityStore;
 * this store only holds: selected dialog, composer text, pagination state.
 */
import { create } from "zustand";
export const useChatStore = create((set) => ({
    selectedPeerId: null,
    selectedIsGroup: false,
    composerText: "",
    loadingHistory: false,
    historyFinished: false,
    setSelectedConversation: (peerId, isGroup) => set({ selectedPeerId: peerId, selectedIsGroup: isGroup, composerText: "", historyFinished: false }),
    setComposerText: (text) => set({ composerText: text }),
    setLoadingHistory: (v) => set({ loadingHistory: v }),
    setHistoryFinished: (v) => set({ historyFinished: v }),
    reset: () => set({ selectedPeerId: null, selectedIsGroup: false, composerText: "", loadingHistory: false, historyFinished: false }),
}));
