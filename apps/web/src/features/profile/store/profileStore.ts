/** Profile store — current user profile editing state */
import { create } from "zustand"
import type { UserProfile } from "@/core/session/sessionTypes"

interface ProfileStore {
  editMode: boolean
  loading: boolean
  error: string | null
  setEditMode(v: boolean): void
  setLoading(v: boolean): void
  setError(msg: string | null): void
}

export const useProfileStore = create<ProfileStore>((set) => ({
  editMode: false,
  loading: false,
  error: null,
  setEditMode: (v) => set({ editMode: v }),
  setLoading: (v) => set({ loading: v }),
  setError: (msg) => set({ error: msg }),
}))
