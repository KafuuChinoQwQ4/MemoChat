import { useEffect, type ReactNode } from "react"
import { useSettingsStore } from "@/features/settings/store/settingsStore"

export function ThemeProvider({ children }: { children: ReactNode }) {
  const theme = useSettingsStore((s) => s.theme)
  const blurEnabled = useSettingsStore((s) => s.blurEnabled)

  useEffect(() => {
    document.documentElement.setAttribute("data-theme", theme)
  }, [theme])

  useEffect(() => {
    if (!blurEnabled) {
      document.documentElement.classList.add("no-blur")
    } else {
      document.documentElement.classList.remove("no-blur")
    }
  }, [blurEnabled])

  return <>{children}</>
}
