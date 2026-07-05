/** Contact feature store — search state + friend list UI */
import { create } from "zustand"

interface ContactStore {
  searchQuery: string
  loading: boolean
  error: string | null
  setSearchQuery(q: string): void
  setLoading(v: boolean): void
  setError(msg: string | null): void
  reset(): void
}

export const useContactStore = create<ContactStore>((set) => ({
  searchQuery: "",
  loading: false,
  error: null,
  setSearchQuery: (q) => set({ searchQuery: q }),
  setLoading: (v) => set({ loading: v }),
  setError: (msg) => set({ error: msg }),
  reset: () => set({ searchQuery: "", loading: false, error: null }),
}))
