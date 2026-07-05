import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/**
 * AppShell — authenticated shell layout.
 * 72px glass sidebar + feature outlet area.
 */
import { useEffect, useRef } from "react";
import { Outlet } from "react-router-dom";
import { IconSidebar } from "./IconSidebar";
import { useGateway } from "@/app/providers/GatewayProvider";
import { registerChatRoutes } from "@/app/dispatch/registerChatRoutes";
import { startConnectionCoordinator } from "@/app/bootstrap/connectionCoordinator";
export function AppShell() {
    const gateway = useGateway();
    const routesRegistered = useRef(false);
    useEffect(() => {
        if (routesRegistered.current)
            return;
        routesRegistered.current = true;
        const unregisterRoutes = registerChatRoutes(gateway.dispatcher);
        const stopCoordinator = startConnectionCoordinator();
        return () => {
            unregisterRoutes();
            stopCoordinator();
        };
    }, [gateway]);
    return (_jsxs("div", { style: { display: "flex", height: "100%", overflow: "hidden" }, children: [_jsx("nav", { style: {
                    width: "var(--sidebar-w)",
                    flexShrink: 0,
                    display: "flex",
                    flexDirection: "column",
                    alignItems: "center",
                    padding: "10px 0 8px",
                    gap: 4,
                    background: "var(--sidebar-bg)",
                    backdropFilter: "blur(24px) saturate(1.6)",
                    WebkitBackdropFilter: "blur(24px) saturate(1.6)",
                    borderRight: "1px solid var(--sidebar-border)",
                    /* Subtle inner glow on right edge */
                    boxShadow: "inset -1px 0 0 rgba(255,255,255,0.12)",
                    zIndex: 10,
                }, children: _jsx(IconSidebar, {}) }), _jsx("div", { style: { flex: 1, overflow: "hidden", display: "flex" }, children: _jsx(Outlet, {}) })] }));
}
