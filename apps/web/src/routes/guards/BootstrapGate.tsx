/**
 * BootstrapGate — shows a loading spinner while post-login bootstrap /
 * session restore runs. Only blocks child render, not navigation itself.
 */
import { Outlet } from "react-router-dom"
import { useSessionStore } from "@/core/session/sessionStore"
import { Spinner } from "@/shared/ui/primitives/Spinner"

export function BootstrapGate() {
  const connState = useSessionStore((s) => s.connState)
  if (
    connState === "connecting" ||
    connState === "chat_login" ||
    connState === "reconnecting"
  ) {
    return (
      <div style={{
        height: "100%",
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        gap: 12,
        color: "var(--text-secondary)",
        fontSize: 14,
      }}>
        <Spinner size={28} />
        <span>{connState === "reconnecting" ? "正在恢复登录…" : "正在连接…"}</span>
      </div>
    )
  }
  return <Outlet />
}
