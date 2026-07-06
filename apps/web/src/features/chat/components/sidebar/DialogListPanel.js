import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** DialogListPanel — scrollable list of conversations */
import { useMemo } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { useChatStore } from "@/features/chat/store/chatStore";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { Badge } from "@/shared/ui/primitives/Badge";
import { formatMessageTime } from "@/shared/lib/time";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
export function DialogListPanel() {
    // Subscribe to the dialogs Map (stable reference), sort in useMemo.
    // Never call getDialogList() directly in a selector — it returns a new
    // array every call, causing Zustand to force an infinite re-render loop.
    const dialogsMap = useEntityStore((s) => s.dialogs);
    const dialogs = useMemo(() => Array.from(dialogsMap.values()).sort((a, b) => {
        if (a.isPinned !== b.isPinned)
            return a.isPinned ? -1 : 1;
        return b.lastMsgTime - a.lastMsgTime;
    }), [dialogsMap]);
    const { selectedPeerId, setSelectedConversation } = useChatStore();
    return (_jsxs(GlassScrollArea, { style: { height: "100%", display: "flex", flexDirection: "column" }, children: [_jsx("div", { style: { padding: "12px 12px 8px", fontWeight: 600, fontSize: 15 }, children: "\u6D88\u606F" }), dialogs.map((d) => {
                const isActive = d.peerId === selectedPeerId;
                const title = d.title ?? d.peerId.toString();
                return (_jsxs("button", { onClick: () => setSelectedConversation(d.peerId, d.isGroup), style: {
                        display: "flex",
                        alignItems: "center",
                        gap: 10,
                        padding: "8px 12px",
                        border: "none",
                        cursor: "pointer",
                        background: isActive ? "var(--tint-selected)" : "transparent",
                        textAlign: "left",
                        width: "100%",
                        borderRadius: 8,
                        transition: "background 100ms ease",
                    }, children: [_jsx(Avatar, { src: d.avatar, name: title, size: 40 }), _jsxs("div", { style: { flex: 1, overflow: "hidden" }, children: [_jsxs("div", { style: { display: "flex", justifyContent: "space-between", alignItems: "center" }, children: [_jsx("span", { style: { fontWeight: 500, fontSize: 14, color: "var(--text-primary)" }, children: title }), _jsx("span", { style: { fontSize: 11, color: "var(--text-disabled)" }, children: formatMessageTime(d.lastMsgTime) })] }), _jsx("div", { style: { fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: d.draftText ? _jsxs("span", { style: { color: "var(--color-badge)" }, children: ["\u8349\u7A3F: ", d.draftText] }) : d.lastMsgContent })] }), d.unreadCount > 0 && _jsx(Badge, { count: d.unreadCount })] }, d.dialogId ?? `${d.isGroup ? "g" : "u"}_${d.peerId}`));
            }), dialogs.length === 0 && (_jsx("div", { style: { textAlign: "center", color: "var(--text-disabled)", fontSize: 13, padding: 32 }, children: "\u6682\u65E0\u6D88\u606F" }))] }));
}
