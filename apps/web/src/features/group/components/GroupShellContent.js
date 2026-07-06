import { jsx as _jsx, jsxs as _jsxs, Fragment as _Fragment } from "react/jsx-runtime";
/** Group feature shell content */
import { useEffect, useMemo, useState } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
function roleLabel(role) {
    if (role === "owner")
        return "群主";
    if (role === "admin")
        return "管理员";
    return "成员";
}
export function GroupShellContent() {
    const groupsMap = useEntityStore((s) => s.groups);
    const groups = useMemo(() => Array.from(groupsMap.values()), [groupsMap]);
    const [selectedGroupId, setSelectedGroupId] = useState(null);
    const selectedGroup = groups.find((g) => g.groupId === selectedGroupId) ?? null;
    useEffect(() => {
        if (groups.length === 0) {
            setSelectedGroupId(null);
            return;
        }
        if (selectedGroupId === null || !groups.some((g) => g.groupId === selectedGroupId)) {
            setSelectedGroupId(groups[0]?.groupId ?? null);
        }
    }, [groups, selectedGroupId]);
    return (_jsxs("div", { style: {
            display: "grid",
            gridTemplateColumns: "280px minmax(0, 1fr)",
            height: "100%",
            width: "100%",
            minWidth: 0,
            overflow: "hidden",
        }, children: [_jsxs(GlassScrollArea, { style: { borderRight: "1px solid var(--divider)", padding: "10px 8px" }, children: [_jsx("div", { style: { padding: "6px 10px 12px", fontWeight: 700, fontSize: 16 }, children: "\u7FA4\u7EC4" }), groups.map((g) => (_jsxs("button", { onClick: () => setSelectedGroupId(g.groupId), style: {
                            display: "flex", alignItems: "center", gap: 10,
                            padding: "10px 12px", border: "none",
                            background: g.groupId === selectedGroupId ? "var(--tint-selected)" : "transparent",
                            cursor: "pointer", width: "100%", textAlign: "left", borderRadius: 8,
                            color: "var(--text-primary)",
                        }, children: [_jsx(Avatar, { src: g.icon, name: g.name, size: 38 }), _jsxs("div", { style: { minWidth: 0 }, children: [_jsx("div", { style: { fontWeight: 600, fontSize: 14, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: g.name }), g.memberCount && (_jsxs("div", { style: { fontSize: 12, color: "var(--text-secondary)" }, children: [g.memberCount, " \u4EBA"] }))] })] }, g.groupId))), groups.length === 0 && (_jsx("div", { style: { textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }, children: "\u6682\u65E0\u7FA4\u7EC4" }))] }), _jsx("div", { style: { minWidth: 0, padding: 16, overflow: "hidden" }, children: _jsx(GlassSurface, { elevated: true, style: {
                        height: "100%",
                        padding: 28,
                        display: "flex",
                        flexDirection: "column",
                        color: "var(--text-primary)",
                    }, children: selectedGroup ? (_jsxs(_Fragment, { children: [_jsxs("div", { style: { display: "flex", alignItems: "center", gap: 16 }, children: [_jsx(Avatar, { src: selectedGroup.icon, name: selectedGroup.name, size: 64, style: { boxShadow: "0 10px 30px rgba(0,0,0,0.10)" } }), _jsxs("div", { style: { minWidth: 0 }, children: [_jsx("div", { style: { fontSize: 22, fontWeight: 700, lineHeight: 1.2 }, children: selectedGroup.name }), _jsx("div", { style: { marginTop: 4, fontSize: 13, color: "var(--text-secondary)" }, children: (selectedGroup.memberCount ?? 0) > 0 ? `${selectedGroup.memberCount} 人` : "成员数未知" }), _jsxs("div", { style: { marginTop: 4, fontSize: 12, color: "var(--text-disabled)" }, children: ["\u7FA4\u53F7: ", selectedGroup.groupId] })] })] }), _jsx("div", { style: { height: 1, background: "var(--divider)", margin: "24px 0" } }), _jsxs("div", { style: { display: "grid", gap: 12, maxWidth: 560 }, children: [_jsxs("div", { children: [_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }, children: "\u6211\u7684\u8EAB\u4EFD" }), _jsx("div", { style: { fontSize: 14 }, children: roleLabel(selectedGroup.role) })] }), _jsxs("div", { children: [_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }, children: "\u7FA4\u516C\u544A" }), _jsx("div", { style: { fontSize: 14, lineHeight: 1.6, whiteSpace: "pre-wrap", overflowWrap: "anywhere" }, children: selectedGroup.announcement?.trim() || "暂无公告" })] })] })] })) : (_jsx("div", { style: { margin: "auto", color: "var(--text-disabled)", fontSize: 14 }, children: "\u9009\u62E9\u7FA4\u7EC4\u67E5\u770B\u8BE6\u60C5" })) }) })] }));
}
