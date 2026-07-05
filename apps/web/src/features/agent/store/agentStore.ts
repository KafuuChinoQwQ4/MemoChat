/** Agent store — AI chat session + streaming state */
import { create } from "zustand"

interface AgentStore {
  sessionId: string | null
  streaming: boolean
  currentStreamText: string
  error: string | null
  setSessionId(id: string | null): void
  setStreaming(v: boolean): void
  appendStreamChunk(chunk: string): void
  finalizeStream(): void
  setError(msg: string | null): void
  reset(): void
}

export const useAgentStore = create<AgentStore>((set) => ({
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
}))
