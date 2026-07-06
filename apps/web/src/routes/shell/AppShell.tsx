/**
 * AppShell — authenticated shell layout.
 * 72px glass sidebar + feature outlet area.
 */
import { useEffect } from "react"
import { Outlet } from "react-router-dom"
import { IconSidebar } from "./IconSidebar"
import { useGateway } from "@/app/providers/GatewayProvider"
import { registerChatRoutes } from "@/app/dispatch/registerChatRoutes"
import { startConnectionCoordinator } from "@/app/bootstrap/connectionCoordinator"

export function AppShell() {
  const gateway = useGateway()

  useEffect(() => {
    const unregisterRoutes = registerChatRoutes(gateway.dispatcher)
    const stopCoordinator = startConnectionCoordinator()
    return () => {
      unregisterRoutes()
      stopCoordinator()
    }
  }, [gateway])

  return (
    <div style={{ display: "flex", height: "100%", overflow: "hidden" }}>
      {/* 72px glass icon sidebar */}
      <nav
        style={{
          width: "var(--sidebar-w)",
          flexShrink: 0,
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          padding: "10px 0 8px",
          gap: 4,
          background: "linear-gradient(180deg, rgba(255,255,255,0.075), rgba(255,255,255,0.022)), var(--sidebar-bg)",
          backdropFilter: "blur(24px) saturate(1.6)",
          WebkitBackdropFilter: "blur(24px) saturate(1.6)",
          borderRight: "1px solid var(--sidebar-border)",
          boxShadow: "inset -1px 0 0 rgba(255,255,255,0.16), 12px 0 34px rgba(0,0,0,0.20)",
          zIndex: 10,
        }}
      >
        <IconSidebar />
      </nav>

      {/* Feature content */}
      <div style={{ flex: 1, overflow: "hidden", display: "flex" }}>
        <Outlet />
      </div>
    </div>
  )
}
