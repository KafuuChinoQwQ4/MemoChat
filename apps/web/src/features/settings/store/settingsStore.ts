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
  setTheme(theme: ThemeMode): void
  toggleTheme(): void
  setBlurEnabled(v: boolean): void
  setLocale(locale: string): void
}

export const useSettingsStore = create<SettingsState & SettingsActions>()(
  persist(
    (set, get) => ({
      theme: "light",
      blurEnabled: true,
      locale: "zh-CN",

      setTheme: (theme) => set({ theme }),
      toggleTheme: () => set({ theme: get().theme === "light" ? "dark" : "light" }),
      setBlurEnabled: (v) => set({ blurEnabled: v }),
      setLocale: (locale) => set({ locale }),
    }),
    {
      name: "mc-settings",
      version: 1,
    },
  ),
)
