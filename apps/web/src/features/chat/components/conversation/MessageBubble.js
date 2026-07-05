import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** MessageBubble — displays a single chat message */
import { formatFullTime } from "@/shared/lib/time";
import { useSessionStore } from "@/core/session/sessionStore";
export function MessageBubble({ message }) {
    const myUid = useSessionStore((s) => s.uid);
    const isSelf = message.fromUid === myUid;
    const isRevoked = message.isRevoked === true;
    return (_jsx("div", { style: {
            display: "flex",
            justifyContent: isSelf ? "flex-end" : "flex-start",
            marginBottom: 8,
            padding: "0 12px",
        }, children: _jsxs("div", { style: {
                maxWidth: "65%",
                padding: "8px 12px",
                borderRadius: isSelf ? "16px 16px 4px 16px" : "16px 16px 16px 4px",
                background: isSelf ? "var(--color-brand-green)" : "rgba(255,255,255,0.7)",
                color: isSelf ? "#fff" : "var(--text-primary)",
                fontSize: 14,
                lineHeight: 1.5,
                wordBreak: "break-word",
                position: "relative",
            }, children: [isRevoked ? (_jsx("span", { style: { color: isSelf ? "rgba(255,255,255,0.7)" : "var(--text-disabled)", fontStyle: "italic", fontSize: 13 }, children: "\u6D88\u606F\u5DF2\u64A4\u56DE" })) : (message.content), _jsxs("div", { style: {
                        marginTop: 3,
                        fontSize: 11,
                        color: isSelf ? "rgba(255,255,255,0.65)" : "var(--text-disabled)",
                        textAlign: "right",
                    }, children: [formatFullTime(message.timestamp), isSelf && (_jsx("span", { style: { marginLeft: 4 }, children: message.state === "sending" ? "⏳" : message.state === "failed" ? "❌" : "✓" }))] })] }) }));
}
