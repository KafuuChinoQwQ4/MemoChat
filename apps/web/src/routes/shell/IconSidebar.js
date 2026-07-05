import { jsx as _jsx, jsxs as _jsxs, Fragment as _Fragment } from "react/jsx-runtime";
/**
 * IconSidebar — 72px vertical icon navigation bar.
 * Mirrors desktop QML tab icon bar (chat/contacts/groups/moments/agent/settings).
 */
import { useNavigate, useLocation } from "react-router-dom";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { useSessionStore } from "@/core/session/sessionStore";
import { cn } from "@/shared/lib/classnames";
import { logoutTeardown } from "@/app/bootstrap/logoutTeardown";
const NAV_ITEMS = [
    { path: "/app/chat", label: "消息", icon: "chat" },
    { path: "/app/contacts", label: "联系人", icon: "contacts" },
    { path: "/app/groups", label: "群组", icon: "groups" },
    { path: "/app/moments", label: "朋友圈", icon: "moments" },
    { path: "/app/agent", label: "AI助手", icon: "agent" },
];
function SimpleIcon({ name, size = 22 }) {
    const stroke = "currentColor";
    const common = {
        width: size,
        height: size,
        viewBox: "0 0 24 24",
        fill: "none",
        stroke,
        strokeWidth: 1.8,
        strokeLinecap: "round",
        strokeLinejoin: "round",
        "aria-hidden": true,
    };
    switch (name) {
        case "chat":
            return (_jsxs("svg", { ...common, children: [_jsx("path", { d: "M5.5 7.5h13" }), _jsx("path", { d: "M5.5 11.5h8.5" }), _jsx("path", { d: "M7 17.5h9.2c2 0 3.3-1.3 3.3-3.2V7.8c0-1.9-1.3-3.2-3.3-3.2H7.8c-2 0-3.3 1.3-3.3 3.2v6.4c0 1.4.7 2.5 1.8 3l-.5 2.2 2.2-1.9Z" })] }));
        case "contacts":
            return (_jsxs("svg", { ...common, children: [_jsx("circle", { cx: "10", cy: "8.4", r: "3" }), _jsx("path", { d: "M4.8 18.5c.6-3.1 2.4-4.7 5.2-4.7s4.6 1.6 5.2 4.7" }), _jsx("path", { d: "M15.4 6.3c1.7.3 2.8 1.6 2.8 3.2 0 1.2-.6 2.3-1.5 2.8" }), _jsx("path", { d: "M16.7 14.4c1.5.6 2.5 1.9 2.9 4.1" })] }));
        case "groups":
            return (_jsxs("svg", { ...common, children: [_jsx("path", { d: "M6.5 9.4 12 5.8l5.5 3.6v7.4c0 1-.7 1.7-1.7 1.7H8.2c-1 0-1.7-.7-1.7-1.7Z" }), _jsx("path", { d: "M10 18.4v-4.5h4v4.5" }), _jsx("path", { d: "M4.2 11.1 12 6l7.8 5.1" })] }));
        case "moments":
            return (_jsxs("svg", { ...common, children: [_jsx("circle", { cx: "12", cy: "12", r: "7.2" }), _jsx("path", { d: "M4.8 12h14.4" }), _jsx("path", { d: "M12 4.8c2 2.1 3 4.5 3 7.2s-1 5.1-3 7.2" }), _jsx("path", { d: "M12 4.8c-2 2.1-3 4.5-3 7.2s1 5.1 3 7.2" })] }));
        case "agent":
            return (_jsxs("svg", { ...common, children: [_jsx("rect", { x: "6.2", y: "7.2", width: "11.6", height: "9.8", rx: "3" }), _jsx("path", { d: "M9.5 11.2h.1" }), _jsx("path", { d: "M14.4 11.2h.1" }), _jsx("path", { d: "M10 14.1c1.2.8 2.8.8 4 0" }), _jsx("path", { d: "M12 7.2V4.8" }), _jsx("path", { d: "M8.2 19.2h7.6" })] }));
        case "settings":
            return (_jsxs("svg", { ...common, children: [_jsx("circle", { cx: "12", cy: "12", r: "2.8" }), _jsx("path", { d: "M12 4.7v2" }), _jsx("path", { d: "M12 17.3v2" }), _jsx("path", { d: "m6.8 6.8 1.4 1.4" }), _jsx("path", { d: "m15.8 15.8 1.4 1.4" }), _jsx("path", { d: "M4.7 12h2" }), _jsx("path", { d: "M17.3 12h2" }), _jsx("path", { d: "m6.8 17.2 1.4-1.4" }), _jsx("path", { d: "m15.8 8.2 1.4-1.4" })] }));
        case "logout":
            return (_jsxs("svg", { ...common, children: [_jsx("path", { d: "M10.2 6.2H7.5c-1.1 0-1.9.8-1.9 1.9v7.8c0 1.1.8 1.9 1.9 1.9h2.7" }), _jsx("path", { d: "M12.8 8.4 16.4 12l-3.6 3.6" }), _jsx("path", { d: "M9.8 12h6.4" })] }));
    }
}
/* Inline styles extracted for reuse */
const navBtnBase = {
    position: "relative",
    display: "flex",
    flexDirection: "column",
    alignItems: "center",
    gap: 3,
    padding: "7px 4px",
    borderRadius: 11,
    border: "none",
    cursor: "pointer",
    fontSize: 10,
    width: "100%",
    transition: "background 150ms ease, color 150ms ease, transform 120ms ease",
    outline: "none",
};
export function IconSidebar() {
    const navigate = useNavigate();
    const location = useLocation();
    const profile = useSessionStore((s) => s.profile);
    async function handleLogout() {
        await logoutTeardown();
        void navigate("/login", { replace: true });
    }
    return (_jsxs(_Fragment, { children: [_jsx(Avatar, { src: profile?.icon, name: profile?.name, size: 42, style: { marginBottom: 10 } }), _jsx("div", { style: { flex: 1, display: "flex", flexDirection: "column", gap: 2, width: "100%" }, children: NAV_ITEMS.map((item) => {
                    const active = location.pathname.startsWith(item.path);
                    return (_jsxs("button", { onClick: () => void navigate(item.path), title: item.label, className: cn(active ? "nav-active" : ""), style: {
                            ...navBtnBase,
                            background: active ? "var(--tint-selected)" : "transparent",
                            color: active ? "var(--color-brand-green)" : "var(--text-secondary)",
                        }, onMouseEnter: (e) => {
                            if (!active) {
                                e.currentTarget.style.background = "var(--tint-hover)";
                                e.currentTarget.style.transform = "scale(1.04)";
                            }
                        }, onMouseLeave: (e) => {
                            if (!active) {
                                e.currentTarget.style.background = "transparent";
                                e.currentTarget.style.transform = "scale(1)";
                            }
                        }, children: [active && (_jsx("span", { "aria-hidden": true, style: {
                                    position: "absolute",
                                    left: 0,
                                    top: "50%",
                                    transform: "translateY(-50%)",
                                    width: 3,
                                    height: 18,
                                    borderRadius: "0 3px 3px 0",
                                    background: "var(--color-brand-green)",
                                    boxShadow: "0 0 6px rgba(7,193,96,0.45)",
                                    transition: "height 200ms var(--transition-spring)",
                                } })), _jsx("span", { style: { display: "grid", placeItems: "center", width: 24, height: 24 }, children: _jsx(SimpleIcon, { name: item.icon }) }), _jsx("span", { children: item.label })] }, item.path));
                }) }), _jsx("div", { style: { display: "flex", flexDirection: "column", alignItems: "center", gap: 2 }, children: ["settings", "logout"].map((name) => (_jsx("button", { onClick: name === "logout" ? () => void handleLogout() : () => void navigate("/app/settings"), title: name === "logout" ? "退出登录" : "设置", style: {
                        display: "grid",
                        placeItems: "center",
                        background: "none",
                        border: "none",
                        cursor: "pointer",
                        padding: 7,
                        borderRadius: 10,
                        color: "var(--text-secondary)",
                        transition: "background 150ms ease, color 150ms ease",
                    }, onMouseEnter: (e) => {
                        e.currentTarget.style.background = "var(--tint-hover)";
                        e.currentTarget.style.color = name === "logout" ? "var(--color-danger)" : "var(--text-primary)";
                    }, onMouseLeave: (e) => {
                        e.currentTarget.style.background = "none";
                        e.currentTarget.style.color = "var(--text-secondary)";
                    }, children: _jsx(SimpleIcon, { name: name }) }, name))) })] }));
}
