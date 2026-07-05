/** Moments store — feed/detail/compose UI state */
import { create } from "zustand"

interface MomentsStore {
  composerOpen: boolean
  selectedMomentId: string | null
  setComposerOpen(v: boolean): void
  setSelectedMoment(id: string | null): void
}

export const useMomentsStore = create<MomentsStore>((set) => ({
  composerOpen: false,
  selectedMomentId: null,
  setComposerOpen: (v) => set({ composerOpen: v }),
  setSelectedMoment: (id) => set({ selectedMomentId: id }),
}))
