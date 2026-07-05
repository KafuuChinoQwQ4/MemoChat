/** Moments store — feed/detail/compose UI state */
import { create } from "zustand";
export const useMomentsStore = create((set) => ({
    composerOpen: false,
    selectedMomentId: null,
    setComposerOpen: (v) => set({ composerOpen: v }),
    setSelectedMoment: (id) => set({ selectedMomentId: id }),
}));
