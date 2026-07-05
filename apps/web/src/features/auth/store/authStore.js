/** Auth feature store — UI state for login/register/reset forms */
import { create } from "zustand";
export const useAuthStore = create((set) => ({
    loading: false,
    error: null,
    setLoading: (v) => set({ loading: v }),
    setError: (msg) => set({ error: msg }),
    reset: () => set({ loading: false, error: null }),
}));
