/**
 * settingsStore — persisted settings (theme, blur, locale).
 * Uses Zustand + localStorage for persistence across sessions.
 * Mirrors desktop settings that were scattered in QML files.
 */
import { create } from "zustand"
import { persist } from "zustand/middleware"

export type ThemeMode = "light" | "dark"

interface SettingsState {
  theme: ThemeMode
  blurEnabled: boolean
  locale: string
}

interface SettingsActions {
  setTheme: (theme: ThemeMode) => void
  toggleTheme: () => void
  setBlurEnabled: (v: boolean) => void
  setLocale: (locale: string) => void
}

const defaultSettings: SettingsState = {
  theme: "dark",
  blurEnabled: true,
  locale: "zh-CN",
}

export const useSettingsStore = create<SettingsState & SettingsActions>()(
  persist(
    (set, get) => ({
      ...defaultSettings,

      setTheme: (theme) => set({ theme }),
      toggleTheme: () => set({ theme: get().theme === "light" ? "dark" : "light" }),
      setBlurEnabled: (v) => set({ blurEnabled: v }),
      setLocale: (locale) => set({ locale }),
    }),
    {
      name: "mc-settings",
      version: 2,
      migrate: (persistedState) => {
        const persisted =
          persistedState && typeof persistedState === "object"
            ? (persistedState as Partial<SettingsState>)
            : {}

        return {
          ...defaultSettings,
          ...persisted,
          theme: "dark",
        }
      },
    },
  ),
)
