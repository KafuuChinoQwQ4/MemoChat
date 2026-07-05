import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** ChatShellContent — full chat feature layout: dialog list + conversation pane */
import { useEffect } from "react";
import { useSearchParams } from "react-router-dom";
import { DialogListPanel } from "./sidebar/DialogListPanel";
import { ConversationPane } from "./conversation/ConversationPane";
import { useChatStore } from "@/features/chat/store/chatStore";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
export function ChatShellContent() {
    const [searchParams] = useSearchParams();
    const { selectedPeerId, setSelectedConversation } = useChatStore();
    // Handle ?peer=uid navigation from ports.navigation.jumpToConversation
    useEffect(() => {
        const peer = searchParams.get("peer");
        if (peer)
            setSelectedConversation(Number(peer), false);
    }, [searchParams, setSelectedConversation]);
    return (_jsxs("div", { style: { display: "flex", height: "100%", width: "100%", overflow: "hidden" }, children: [_jsx("div", { style: { width: "var(--list-panel-w)", flexShrink: 0, borderRight: "1px solid var(--divider)", overflow: "hidden" }, children: _jsx(DialogListPanel, {}) }), _jsx("div", { style: { flex: 1, overflow: "hidden" }, children: selectedPeerId !== null ? (_jsx(ConversationPane, { peerId: selectedPeerId })) : (_jsx(GlassSurface, { style: {
                        height: "100%",
                        display: "flex",
                        alignItems: "center",
                        justifyContent: "center",
                        color: "var(--text-disabled)",
                        fontSize: 14,
                    }, children: "\u9009\u62E9\u4E00\u4E2A\u5BF9\u8BDD\u5F00\u59CB\u804A\u5929" })) })] }));
}
