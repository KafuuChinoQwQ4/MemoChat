/** Profile store — current user profile editing state */
import { create } from "zustand";
export const useProfileStore = create((set) => ({
    editMode: false,
    loading: false,
    error: null,
    setEditMode: (v) => set({ editMode: v }),
    setLoading: (v) => set({ loading: v }),
    setError: (msg) => set({ error: msg }),
}));
