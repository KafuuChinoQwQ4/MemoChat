import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** Group feature shell content */
import { useMemo } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
export function GroupShellContent() {
    const groupsMap = useEntityStore((s) => s.groups);
    const groups = useMemo(() => Array.from(groupsMap.values()), [groupsMap]);
    return (_jsxs("div", { style: { display: "flex", height: "100%" }, children: [_jsxs(GlassScrollArea, { style: { width: 250, borderRight: "1px solid var(--divider)", flexShrink: 0 }, children: [_jsx("div", { style: { padding: "12px 12px 8px", fontWeight: 600, fontSize: 15 }, children: "\u7FA4\u7EC4" }), groups.map((g) => (_jsxs("button", { style: {
                            display: "flex", alignItems: "center", gap: 10,
                            padding: "8px 12px", border: "none", background: "transparent",
                            cursor: "pointer", width: "100%", textAlign: "left", borderRadius: 8,
                        }, children: [_jsx(Avatar, { src: g.icon, name: g.name, size: 38 }), _jsxs("div", { children: [_jsx("div", { style: { fontWeight: 500, fontSize: 14 }, children: g.name }), g.memberCount && (_jsxs("div", { style: { fontSize: 12, color: "var(--text-secondary)" }, children: [g.memberCount, " \u4EBA"] }))] })] }, g.groupId))), groups.length === 0 && (_jsx("div", { style: { textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }, children: "\u6682\u65E0\u7FA4\u7EC4" }))] }), _jsx(GlassSurface, { style: { flex: 1, height: "100%", display: "flex", alignItems: "center", justifyContent: "center", color: "var(--text-disabled)", fontSize: 14 }, children: "\u9009\u62E9\u7FA4\u7EC4\u67E5\u770B\u8BE6\u60C5" })] }));
}
