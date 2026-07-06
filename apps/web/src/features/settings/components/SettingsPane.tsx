/** SettingsPane — appearance settings */
import { useSettingsStore } from "@/features/settings/store/settingsStore"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { useSessionStore } from "@/core/session/sessionStore"
import { displayNameWithoutInternalId, publicUserIdText } from "@/core/entities/displayIds"
import { Avatar } from "@/shared/ui/primitives/Avatar"

function SunIcon() {
  return (
    <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor"
      strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" aria-hidden>
      <circle cx="12" cy="12" r="3.2" />
      <path d="M12 4.8v1.6M12 17.6v1.6M6.9 6.9l1.1 1.1M16 16l1.1 1.1M4.8 12h1.6M17.6 12h1.6M6.9 17.1l1.1-1.1M16 8l1.1-1.1" />
    </svg>
  )
}

function MoonIcon() {
  return (
    <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor"
      strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" aria-hidden>
      <path d="M20.2 14.2A7.4 7.4 0 0 1 9.8 3.8 7.4 7.4 0 1 0 20.2 14.2Z" />
    </svg>
  )
}

/* Section wrapper with subtle header */
function Section({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <section style={{ marginBottom: 28 }}>
      <div style={{
        fontSize: 11,
        fontWeight: 600,
        letterSpacing: "0.06em",
        textTransform: "uppercase",
        color: "var(--text-disabled)",
        marginBottom: 12,
      }}>
        {title}
      </div>
      {children}
    </section>
  )
}

/* Row item inside a section card */
function Row({
  label,
  sublabel,
  action,
}: {
  label: string
  sublabel?: string
  action?: React.ReactNode
}) {
  return (
    <div style={{
      display: "flex",
      alignItems: "center",
      justifyContent: "space-between",
      padding: "11px 16px",
      gap: 12,
    }}>
      <div>
        <div style={{ fontSize: 14 }}>{label}</div>
        {sublabel && (
          <div style={{ fontSize: 12, color: "var(--text-secondary)", marginTop: 1 }}>
            {sublabel}
          </div>
        )}
      </div>
      {action}
    </div>
  )
}

export function SettingsPane() {
  const theme        = useSettingsStore((s) => s.theme)
  const blurEnabled  = useSettingsStore((s) => s.blurEnabled)
  const toggleTheme  = useSettingsStore((s) => s.toggleTheme)
  const setBlurEnabled = useSettingsStore((s) => s.setBlurEnabled)
  const profile      = useSessionStore((s) => s.profile)
  const profileName = displayNameWithoutInternalId(profile?.name, profile?.userId, profile?.uid ?? 0, "—")

  return (
    <GlassSurface style={{ height: "100%", overflow: "auto", padding: "24px 28px" }}>
      <h2 style={{
        fontSize: 20,
        fontWeight: 700,
        marginBottom: 28,
        letterSpacing: "-0.3px",
        animation: "fade-up 180ms ease both",
      }}>
        设置
      </h2>

      {/* ── 个人资料 ─────────────────────────────────────────── */}
      <Section title="个人资料">
        <GlassSurface
          elevated
          style={{
            borderRadius: 14,
            overflow: "hidden",
            animation: "fade-up 200ms ease both",
          }}
        >
          <div style={{
            display: "flex",
            alignItems: "center",
            gap: 16,
            padding: "18px 20px",
          }}>
            <Avatar src={profile?.icon} name={profileName} size={56} />
            <div>
              <div style={{ fontWeight: 600, fontSize: 16, marginBottom: 2 }}>
                {profileName}
              </div>
              <div style={{ fontSize: 13, color: "var(--text-secondary)" }}>
                {profile?.email ?? "—"}
              </div>
              <div style={{ fontSize: 11, color: "var(--text-disabled)", marginTop: 3 }}>
                {publicUserIdText(profile?.userId)}
              </div>
            </div>
          </div>
        </GlassSurface>
      </Section>

      {/* ── 外观 ─────────────────────────────────────────────── */}
      <Section title="外观">
        <GlassSurface
          elevated
          style={{
            borderRadius: 14,
            overflow: "hidden",
            animation: "fade-up 220ms ease both",
          }}
        >
          {/* Theme row */}
          <Row
            label="主题模式"
            sublabel={theme === "light" ? "当前：浅色" : "当前：深色"}
            action={
              <GlassButton onClick={toggleTheme}>
                <span style={{ display: "inline-flex", alignItems: "center", gap: 6 }}>
                  {theme === "light" ? <MoonIcon /> : <SunIcon />}
                  {theme === "light" ? "切换深色" : "切换浅色"}
                </span>
              </GlassButton>
            }
          />

          {/* Divider */}
          <div style={{ height: 1, background: "var(--divider)", margin: "0 16px" }} />

          {/* Blur row */}
          <Row
            label="毛玻璃模糊"
            sublabel="需要支持 backdrop-filter 的浏览器"
            action={
              <GlassButton onClick={() => setBlurEnabled(!blurEnabled)}>
                {blurEnabled ? "✓ 已启用" : "✗ 已禁用"}
              </GlassButton>
            }
          />
        </GlassSurface>
      </Section>

      {/* ── 关于 ─────────────────────────────────────────────── */}
      <Section title="关于">
        <GlassSurface
          elevated
          style={{
            borderRadius: 14,
            overflow: "hidden",
            animation: "fade-up 240ms ease both",
          }}
        >
          <Row
            label="MemoChat Web"
            sublabel="v0.1.0 — React + TypeScript + Zustand"
          />
        </GlassSurface>
      </Section>
    </GlassSurface>
  )
}
