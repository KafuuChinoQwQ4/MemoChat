/**
 * IconSidebar — 72px vertical icon navigation bar.
 * Mirrors desktop QML tab icon bar (chat/contacts/groups/moments/agent/settings).
 */
import { useNavigate, useLocation } from "react-router-dom"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { useSessionStore } from "@/core/session/sessionStore"
import { cn } from "@/shared/lib/classnames"
import { logoutTeardown } from "@/app/bootstrap/logoutTeardown"

interface NavItem { path: string; label: string; icon: string }

const NAV_ITEMS: NavItem[] = [
  { path: "/app/chat",     label: "消息", icon: "💬" },
  { path: "/app/contacts", label: "联系人", icon: "👥" },
  { path: "/app/groups",   label: "群组", icon: "🏘️" },
  { path: "/app/moments",  label: "朋友圈", icon: "🌐" },
  { path: "/app/agent",    label: "AI助手", icon: "🤖" },
]

export function IconSidebar() {
  const navigate = useNavigate()
  const location = useLocation()
  const profile = useSessionStore((s) => s.profile)

  async function handleLogout() {
    await logoutTeardown()
    navigate("/login", { replace: true })
  }

  return (
    <>
      {/* User avatar at top */}
      <Avatar src={profile?.icon} name={profile?.name} size={44} style={{ marginBottom: 8 } as React.CSSProperties} />

      {/* Navigation items */}
      <div style={{ flex: 1, display: "flex", flexDirection: "column", gap: 4, width: "100%" }}>
        {NAV_ITEMS.map((item) => {
          const active = location.pathname.startsWith(item.path)
          return (
            <button
              key={item.path}
              onClick={() => navigate(item.path)}
              title={item.label}
              style={{
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
                gap: 3,
                padding: "6px 4px",
                borderRadius: 10,
                border: "none",
                cursor: "pointer",
                background: active ? "var(--tint-selected)" : "transparent",
                color: active ? "var(--color-brand-green)" : "var(--text-secondary)",
                fontSize: 22,
                width: "100%",
                transition: "background 110ms ease",
              }}
              className={cn(active ? "nav-active" : "")}
            >
              <span>{item.icon}</span>
              <span style={{ fontSize: 10 }}>{item.label}</span>
            </button>
          )
        })}
      </div>

      {/* Settings + Logout at bottom */}
      <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 4 }}>
        <button
          onClick={() => navigate("/app/settings")}
          title="设置"
          style={{ background: "none", border: "none", cursor: "pointer", fontSize: 22, padding: 6, borderRadius: 10, color: "var(--text-secondary)" }}
        >
          ⚙️
        </button>
        <button
          onClick={() => { void handleLogout() }}
          title="退出登录"
          style={{ background: "none", border: "none", cursor: "pointer", fontSize: 20, padding: 6, borderRadius: 10, color: "var(--text-secondary)" }}
        >
          🚪
        </button>
      </div>
    </>
  )
}
