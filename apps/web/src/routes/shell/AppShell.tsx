/**
 * AppShell — the main authenticated shell layout.
 * Three-column grid: icon sidebar | list panel (injected by active tab) | content.
 * Registers chat routes once on mount.
 */
import { useEffect, useRef } from "react"
import { Outlet } from "react-router-dom"
import { ThreeColumnShell } from "@/shared/ui/layout/ThreeColumnShell"
import { IconSidebar } from "./IconSidebar"
import { useGateway } from "@/app/providers/GatewayProvider"
import { registerChatRoutes } from "@/app/dispatch/registerChatRoutes"
import { startConnectionCoordinator } from "@/app/bootstrap/connectionCoordinator"

export function AppShell() {
  const gateway = useGateway()
  const routesRegistered = useRef(false)

  useEffect(() => {
    if (routesRegistered.current) return
    routesRegistered.current = true
    // Wire incoming opcodes → entity store actions
    const unregisterRoutes = registerChatRoutes(gateway.dispatcher)
    // Watch WS reconnect events
    const stopCoordinator = startConnectionCoordinator()
    return () => {
      unregisterRoutes()
      stopCoordinator()
    }
  }, [gateway])

  // Each feature tab renders its own list panel + content area.
  // AppShell only provides the 72px icon sidebar; the Outlet fills the remaining space.
  return (
    <div style={{ display: "flex", height: "100%", overflow: "hidden" }}>
      {/* 72px icon sidebar */}
      <nav style={{
        width: "var(--sidebar-w)",
        flexShrink: 0,
        display: "flex",
        flexDirection: "column",
        alignItems: "center",
        padding: "8px 0",
        gap: 4,
        background: "rgba(255,255,255,0.08)",
        borderRight: "1px solid var(--divider)",
      }}>
        <IconSidebar />
      </nav>

      {/* Feature content — fills remaining width, manages its own list panel + content split */}
      <div style={{ flex: 1, overflow: "hidden", display: "flex" }}>
        <Outlet />
      </div>
    </div>
  )
}
