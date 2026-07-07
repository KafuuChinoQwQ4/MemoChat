import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** DialogListPanel — scrollable list of conversations */
import { useMemo } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { displayNameWithoutInternalId } from "@/core/entities/displayIds";
import { useChatStore } from "@/features/chat/store/chatStore";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { Badge } from "@/shared/ui/primitives/Badge";
import { formatMessageTime } from "@/shared/lib/time";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
export function DialogListPanel() {
    const dialogsMap = useEntityStore((s) => s.dialogs);
    const dialogs = useMemo(() => Array.from(dialogsMap.values()).sort((a, b) => {
        if (a.isPinned !== b.isPinned)
            return a.isPinned ? -1 : 1;
        return b.lastMsgTime - a.lastMsgTime;
    }), [dialogsMap]);
    const selectedPeerId = useChatStore((s) => s.selectedPeerId);
    return (_jsxs(GlassScrollArea, { style: { height: "100%", display: "flex", flexDirection: "column" }, children: [_jsx("div", { style: {
                    padding: "14px 14px 10px",
                    fontWeight: 700,
                    fontSize: 16,
                    color: "var(--text-primary)",
                    letterSpacing: "-0.01em",
                    flexShrink: 0,
                }, children: "\u6D88\u606F" }), dialogs.map((d) => {
                const isActive = d.peerId === selectedPeerId;
                const title = d.isGroup
                    ? displayNameWithoutInternalId(d.title, undefined, d.peerId, "群聊")
                    : displayNameWithoutInternalId(d.title, undefined, d.peerId, "未知用户");
                return (_jsxs("button", { onClick: () => useChatStore.getState().setSelectedConversation(d.peerId, d.isGroup), style: {
                        display: "flex",
                        alignItems: "center",
                        gap: 10,
                        padding: "9px 12px",
                        margin: "1px 6px",
                        border: "none",
                        cursor: "pointer",
                        background: isActive ? "var(--tint-selected)" : "transparent",
                        textAlign: "left",
                        width: "calc(100% - 12px)",
                        borderRadius: 10,
                        transition: "background 120ms ease",
                        flexShrink: 0,
                    }, onMouseEnter: (e) => {
                        if (!isActive)
                            e.currentTarget.style.background = "var(--dialog-item-hover)";
                    }, onMouseLeave: (e) => {
                        if (!isActive)
                            e.currentTarget.style.background = "transparent";
                    }, children: [_jsx(Avatar, { src: d.avatar, name: title, size: 42 }), _jsxs("div", { style: { flex: 1, overflow: "hidden" }, children: [_jsxs("div", { style: {
                                        display: "flex",
                                        justifyContent: "space-between",
                                        alignItems: "baseline",
                                        gap: 4,
                                        marginBottom: 2,
                                    }, children: [_jsx("span", { style: {
                                                fontWeight: 500,
                                                fontSize: 14,
                                                color: "var(--text-primary)",
                                                overflow: "hidden",
                                                textOverflow: "ellipsis",
                                                whiteSpace: "nowrap",
                                                flex: 1,
                                            }, children: title }), _jsx("span", { style: {
                                                fontSize: 11,
                                                color: "var(--text-disabled)",
                                                flexShrink: 0,
                                            }, children: formatMessageTime(d.lastMsgTime) })] }), _jsx("div", { style: {
                                        fontSize: 12,
                                        color: "var(--text-secondary)",
                                        overflow: "hidden",
                                        textOverflow: "ellipsis",
                                        whiteSpace: "nowrap",
                                        lineHeight: 1.4,
                                    }, children: d.draftText
                                        ? _jsxs("span", { style: { color: "var(--color-badge)" }, children: ["\u8349\u7A3F: ", d.draftText] })
                                        : d.lastMsgContent })] }), d.unreadCount > 0 && _jsx(Badge, { count: d.unreadCount })] }, d.dialogId ?? `${d.isGroup ? "g" : "u"}_${d.peerId}`));
            }), dialogs.length === 0 && (_jsx("div", { style: {
                    textAlign: "center",
                    color: "var(--text-disabled)",
                    fontSize: 13,
                    padding: "40px 16px",
                    lineHeight: 1.6,
                }, children: "\u6682\u65E0\u6D88\u606F" }))] }));
}
