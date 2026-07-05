/** Agent store — AI chat session + streaming state */
import { create } from "zustand";
export const useAgentStore = create((set) => ({
    sessionId: null,
    streaming: false,
    currentStreamText: "",
    error: null,
    setSessionId: (id) => set({ sessionId: id }),
    setStreaming: (v) => set({ streaming: v }),
    appendStreamChunk: (chunk) => set((s) => ({ currentStreamText: s.currentStreamText + chunk })),
    finalizeStream: () => set({ streaming: false }),
    setError: (msg) => set({ error: msg, streaming: false }),
    reset: () => set({ sessionId: null, streaming: false, currentStreamText: "", error: null }),
}));
