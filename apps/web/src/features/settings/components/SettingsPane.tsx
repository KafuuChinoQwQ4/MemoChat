/** SettingsPane — theme/blur/locale settings UI */
import { useSettingsStore } from "@/features/settings/store/settingsStore"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { useSessionStore } from "@/core/session/sessionStore"
import { Avatar } from "@/shared/ui/primitives/Avatar"

export function SettingsPane() {
  const { theme, toggleTheme, blurEnabled, setBlurEnabled } = useSettingsStore()
  const profile = useSessionStore((s) => s.profile)

  return (
    <GlassSurface style={{ height: "100%", overflow: "auto", padding: 24 }}>
      <h2 style={{ fontSize: 18, fontWeight: 700, marginBottom: 20 }}>设置</h2>

      {/* Profile section */}
      <section style={{ marginBottom: 24 }}>
        <h3 style={{ fontSize: 14, fontWeight: 600, marginBottom: 12, color: "var(--text-secondary)" }}>个人资料</h3>
        <div style={{ display: "flex", alignItems: "center", gap: 14 }}>
          <Avatar src={profile?.icon} name={profile?.name} size={56} />
          <div>
            <div style={{ fontWeight: 600, fontSize: 16 }}>{profile?.name ?? "—"}</div>
            <div style={{ fontSize: 13, color: "var(--text-secondary)" }}>{profile?.email ?? "—"}</div>
            <div style={{ fontSize: 12, color: "var(--text-disabled)", marginTop: 2 }}>UID: {profile?.uid ?? "—"}</div>
          </div>
        </div>
      </section>

      {/* Appearance section */}
      <section style={{ marginBottom: 24 }}>
        <h3 style={{ fontSize: 14, fontWeight: 600, marginBottom: 12, color: "var(--text-secondary)" }}>外观</h3>
        <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
          <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
            <span style={{ fontSize: 14 }}>主题模式</span>
            <GlassButton onClick={toggleTheme}>
              {theme === "light" ? "🌙 深色" : "☀️ 浅色"}
            </GlassButton>
          </div>
          <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
            <span style={{ fontSize: 14 }}>毛玻璃模糊效果</span>
            <GlassButton onClick={() => setBlurEnabled(!blurEnabled)}>
              {blurEnabled ? "✓ 已启用" : "✗ 已禁用"}
            </GlassButton>
          </div>
        </div>
      </section>

      {/* About */}
      <section>
        <h3 style={{ fontSize: 14, fontWeight: 600, marginBottom: 8, color: "var(--text-secondary)" }}>关于</h3>
        <div style={{ fontSize: 13, color: "var(--text-secondary)" }}>
          MemoChat Web v0.1.0 — 基于 React + TypeScript + Zustand
        </div>
      </section>
    </GlassSurface>
  )
}
