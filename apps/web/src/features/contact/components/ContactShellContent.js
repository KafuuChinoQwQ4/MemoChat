import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** ContactShellContent — friend list + apply list view */
import { useMemo } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
export function ContactShellContent() {
    const friendsMap = useEntityStore((s) => s.friends);
    const friends = useMemo(() => Array.from(friendsMap.values()), [friendsMap]);
    return (_jsxs("div", { style: { display: "flex", height: "100%" }, children: [_jsxs(GlassScrollArea, { style: { width: 250, borderRight: "1px solid var(--divider)", flexShrink: 0 }, children: [_jsx("div", { style: { padding: "12px 12px 8px", fontWeight: 600, fontSize: 15 }, children: "\u8054\u7CFB\u4EBA" }), friends.map((f) => (_jsxs("button", { style: {
                            display: "flex",
                            alignItems: "center",
                            gap: 10,
                            padding: "8px 12px",
                            border: "none",
                            background: "transparent",
                            cursor: "pointer",
                            width: "100%",
                            textAlign: "left",
                            borderRadius: 8,
                        }, children: [_jsx(Avatar, { src: f.icon, name: f.name, size: 38 }), _jsxs("div", { children: [_jsx("div", { style: { fontWeight: 500, fontSize: 14 }, children: f.name }), _jsx("div", { style: { fontSize: 12, color: "var(--text-secondary)" }, children: f.email })] })] }, f.uid))), friends.length === 0 && (_jsx("div", { style: { textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }, children: "\u6682\u65E0\u597D\u53CB" }))] }), _jsx(GlassSurface, { style: { flex: 1, height: "100%", display: "flex", alignItems: "center", justifyContent: "center", color: "var(--text-disabled)", fontSize: 14 }, children: "\u9009\u62E9\u8054\u7CFB\u4EBA\u67E5\u770B\u8BE6\u60C5" })] }));
}
