import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** SettingsPane — appearance settings */
import { useSettingsStore } from "@/features/settings/store/settingsStore";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { useSessionStore } from "@/core/session/sessionStore";
import { displayNameWithoutInternalId, publicUserIdText } from "@/core/entities/displayIds";
import { Avatar } from "@/shared/ui/primitives/Avatar";
function SunIcon() {
    return (_jsxs("svg", { width: "15", height: "15", viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: "1.8", strokeLinecap: "round", strokeLinejoin: "round", "aria-hidden": true, children: [_jsx("circle", { cx: "12", cy: "12", r: "3.2" }), _jsx("path", { d: "M12 4.8v1.6M12 17.6v1.6M6.9 6.9l1.1 1.1M16 16l1.1 1.1M4.8 12h1.6M17.6 12h1.6M6.9 17.1l1.1-1.1M16 8l1.1-1.1" })] }));
}
function MoonIcon() {
    return (_jsx("svg", { width: "15", height: "15", viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: "1.8", strokeLinecap: "round", strokeLinejoin: "round", "aria-hidden": true, children: _jsx("path", { d: "M20.2 14.2A7.4 7.4 0 0 1 9.8 3.8 7.4 7.4 0 1 0 20.2 14.2Z" }) }));
}
/* Section wrapper with subtle header */
function Section({ title, children }) {
    return (_jsxs("section", { style: { marginBottom: 28 }, children: [_jsx("div", { style: {
                    fontSize: 11,
                    fontWeight: 600,
                    letterSpacing: "0.06em",
                    textTransform: "uppercase",
                    color: "var(--text-disabled)",
                    marginBottom: 12,
                }, children: title }), children] }));
}
/* Row item inside a section card */
function Row({ label, sublabel, action, }) {
    return (_jsxs("div", { style: {
            display: "flex",
            alignItems: "center",
            justifyContent: "space-between",
            padding: "11px 16px",
            gap: 12,
        }, children: [_jsxs("div", { children: [_jsx("div", { style: { fontSize: 14 }, children: label }), sublabel && (_jsx("div", { style: { fontSize: 12, color: "var(--text-secondary)", marginTop: 1 }, children: sublabel }))] }), action] }));
}
export function SettingsPane() {
    const theme = useSettingsStore((s) => s.theme);
    const blurEnabled = useSettingsStore((s) => s.blurEnabled);
    const toggleTheme = useSettingsStore((s) => s.toggleTheme);
    const setBlurEnabled = useSettingsStore((s) => s.setBlurEnabled);
    const profile = useSessionStore((s) => s.profile);
    const profileName = displayNameWithoutInternalId(profile?.name, profile?.userId, profile?.uid ?? 0, "—");
    return (_jsxs(GlassSurface, { style: { height: "100%", overflow: "auto", padding: "24px 28px" }, children: [_jsx("h2", { style: {
                    fontSize: 20,
                    fontWeight: 700,
                    marginBottom: 28,
                    letterSpacing: "-0.3px",
                    animation: "fade-up 180ms ease both",
                }, children: "\u8BBE\u7F6E" }), _jsx(Section, { title: "\u4E2A\u4EBA\u8D44\u6599", children: _jsx(GlassSurface, { elevated: true, style: {
                        borderRadius: 14,
                        overflow: "hidden",
                        animation: "fade-up 200ms ease both",
                    }, children: _jsxs("div", { style: {
                            display: "flex",
                            alignItems: "center",
                            gap: 16,
                            padding: "18px 20px",
                        }, children: [_jsx(Avatar, { src: profile?.icon, name: profileName, size: 56 }), _jsxs("div", { children: [_jsx("div", { style: { fontWeight: 600, fontSize: 16, marginBottom: 2 }, children: profileName }), _jsx("div", { style: { fontSize: 13, color: "var(--text-secondary)" }, children: profile?.email ?? "—" }), _jsx("div", { style: { fontSize: 11, color: "var(--text-disabled)", marginTop: 3 }, children: publicUserIdText(profile?.userId) })] })] }) }) }), _jsx(Section, { title: "\u5916\u89C2", children: _jsxs(GlassSurface, { elevated: true, style: {
                        borderRadius: 14,
                        overflow: "hidden",
                        animation: "fade-up 220ms ease both",
                    }, children: [_jsx(Row, { label: "\u4E3B\u9898\u6A21\u5F0F", sublabel: theme === "light" ? "当前：浅色" : "当前：深色", action: _jsx(GlassButton, { onClick: toggleTheme, children: _jsxs("span", { style: { display: "inline-flex", alignItems: "center", gap: 6 }, children: [theme === "light" ? _jsx(MoonIcon, {}) : _jsx(SunIcon, {}), theme === "light" ? "切换深色" : "切换浅色"] }) }) }), _jsx("div", { style: { height: 1, background: "var(--divider)", margin: "0 16px" } }), _jsx(Row, { label: "\u6BDB\u73BB\u7483\u6A21\u7CCA", sublabel: "\u9700\u8981\u652F\u6301 backdrop-filter \u7684\u6D4F\u89C8\u5668", action: _jsx(GlassButton, { onClick: () => setBlurEnabled(!blurEnabled), children: blurEnabled ? "✓ 已启用" : "✗ 已禁用" }) })] }) }), _jsx(Section, { title: "\u5173\u4E8E", children: _jsx(GlassSurface, { elevated: true, style: {
                        borderRadius: 14,
                        overflow: "hidden",
                        animation: "fade-up 240ms ease both",
                    }, children: _jsx(Row, { label: "MemoChat Web", sublabel: "v0.1.0 \u2014 React + TypeScript + Zustand" }) }) })] }));
}
