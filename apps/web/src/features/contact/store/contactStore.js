/** Contact feature store — search state + friend list UI */
import { create } from "zustand";
export const useContactStore = create((set) => ({
    searchQuery: "",
    loading: false,
    error: null,
    setSearchQuery: (q) => set({ searchQuery: q }),
    setLoading: (v) => set({ loading: v }),
    setError: (msg) => set({ error: msg }),
    reset: () => set({ searchQuery: "", loading: false, error: null }),
}));
