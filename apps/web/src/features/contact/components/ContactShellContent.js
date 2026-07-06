import { jsx as _jsx, jsxs as _jsxs, Fragment as _Fragment } from "react/jsx-runtime";
/** ContactShellContent — friend list + apply list view */
import { useEffect, useMemo, useState } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
export function ContactShellContent() {
    const friendsMap = useEntityStore((s) => s.friends);
    const friends = useMemo(() => Array.from(friendsMap.values()), [friendsMap]);
    const [selectedUid, setSelectedUid] = useState(null);
    const selectedFriend = friends.find((f) => f.uid === selectedUid) ?? null;
    useEffect(() => {
        if (friends.length === 0) {
            setSelectedUid(null);
            return;
        }
        if (selectedUid === null || !friends.some((f) => f.uid === selectedUid)) {
            setSelectedUid(friends[0]?.uid ?? null);
        }
    }, [friends, selectedUid]);
    return (_jsxs("div", { style: {
            display: "grid",
            gridTemplateColumns: "280px minmax(0, 1fr)",
            height: "100%",
            width: "100%",
            minWidth: 0,
            overflow: "hidden",
        }, children: [_jsxs(GlassScrollArea, { style: { borderRight: "1px solid var(--divider)", padding: "10px 8px" }, children: [_jsx("div", { style: { padding: "6px 10px 12px", fontWeight: 700, fontSize: 16 }, children: "\u8054\u7CFB\u4EBA" }), friends.map((f) => (_jsxs("button", { onClick: () => setSelectedUid(f.uid), style: {
                            display: "flex",
                            alignItems: "center",
                            gap: 10,
                            padding: "10px 12px",
                            border: "none",
                            background: f.uid === selectedUid ? "var(--tint-selected)" : "transparent",
                            cursor: "pointer",
                            width: "100%",
                            textAlign: "left",
                            borderRadius: 10,
                            color: "var(--text-primary)",
                            transition: "background 140ms ease, transform 120ms ease",
                        }, children: [_jsx(Avatar, { src: f.icon, name: f.name, size: 38 }), _jsxs("div", { style: { minWidth: 0 }, children: [_jsx("div", { style: { fontWeight: 600, fontSize: 14, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: f.name }), _jsx("div", { style: { fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: f.email })] })] }, f.uid))), friends.length === 0 && (_jsx("div", { style: { textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }, children: "\u6682\u65E0\u597D\u53CB" }))] }), _jsx("div", { style: { minWidth: 0, padding: 16, overflow: "hidden" }, children: _jsx(GlassSurface, { elevated: true, style: {
                        height: "100%",
                        padding: 28,
                        display: "flex",
                        flexDirection: "column",
                        color: "var(--text-primary)",
                    }, children: selectedFriend ? (_jsxs(_Fragment, { children: [_jsxs("div", { style: { display: "flex", alignItems: "center", gap: 16 }, children: [_jsx(Avatar, { src: selectedFriend.icon, name: selectedFriend.name, size: 64, style: { boxShadow: "0 10px 30px rgba(0,0,0,0.10)" } }), _jsxs("div", { style: { minWidth: 0 }, children: [_jsx("div", { style: { fontSize: 22, fontWeight: 700, lineHeight: 1.2 }, children: selectedFriend.name }), _jsx("div", { style: { marginTop: 4, fontSize: 13, color: "var(--text-secondary)" }, children: selectedFriend.email }), _jsxs("div", { style: { marginTop: 4, fontSize: 12, color: "var(--text-disabled)" }, children: ["UID: ", selectedFriend.uid] })] })] }), _jsx("div", { style: { height: 1, background: "var(--divider)", margin: "24px 0" } }), _jsxs("div", { style: { display: "grid", gap: 12, maxWidth: 520 }, children: [_jsxs("div", { children: [_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }, children: "\u5907\u6CE8" }), _jsx("div", { style: { fontSize: 14 }, children: selectedFriend.desc?.trim() || "暂无备注" })] }), _jsxs("div", { children: [_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }, children: "\u8D26\u53F7\u72B6\u6001" }), _jsx("div", { style: { fontSize: 14 }, children: "\u5DF2\u6DFB\u52A0\u4E3A\u597D\u53CB" })] })] })] })) : (_jsx("div", { style: { margin: "auto", color: "var(--text-disabled)", fontSize: 14 }, children: "\u9009\u62E9\u8054\u7CFB\u4EBA\u67E5\u770B\u8BE6\u60C5" })) }) })] }));
}
