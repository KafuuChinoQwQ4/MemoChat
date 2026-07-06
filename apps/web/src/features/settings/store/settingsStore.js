/**
 * settingsStore — persisted settings (theme, blur, locale).
 * Uses Zustand + localStorage for persistence across sessions.
 * Mirrors desktop settings that were scattered in QML files.
 */
import { create } from "zustand";
import { persist } from "zustand/middleware";
const defaultSettings = {
    theme: "dark",
    blurEnabled: true,
    locale: "zh-CN",
};
export const useSettingsStore = create()(persist((set, get) => ({
    ...defaultSettings,
    setTheme: (theme) => set({ theme }),
    toggleTheme: () => set({ theme: get().theme === "light" ? "dark" : "light" }),
    setBlurEnabled: (v) => set({ blurEnabled: v }),
    setLocale: (locale) => set({ locale }),
}), {
    name: "mc-settings",
    version: 2,
    migrate: (persistedState) => {
        const persisted = persistedState && typeof persistedState === "object"
            ? persistedState
            : {};
        return {
            ...defaultSettings,
            ...persisted,
            theme: "dark",
        };
    },
}));
