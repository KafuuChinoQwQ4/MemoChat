/** Auth feature store — UI state for login/register/reset forms */
import { create } from "zustand"

interface AuthStore {
  loading: boolean
  error: string | null
  setLoading(v: boolean): void
  setError(msg: string | null): void
  reset(): void
}

export const useAuthStore = create<AuthStore>((set) => ({
  loading: false,
  error: null,
  setLoading: (v) => set({ loading: v }),
  setError: (msg) => set({ error: msg }),
  reset: () => set({ loading: false, error: null }),
}))
