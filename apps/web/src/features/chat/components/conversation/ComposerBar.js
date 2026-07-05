import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** ComposerBar — text input + send button for chat messages */
import { useRef } from "react";
import { useChatStore } from "@/features/chat/store/chatStore";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
export function ComposerBar({ onSend }) {
    const { composerText, setComposerText } = useChatStore();
    const textareaRef = useRef(null);
    function handleKeyDown(e) {
        if (e.key === "Enter" && !e.shiftKey) {
            e.preventDefault();
            submit();
        }
    }
    function submit() {
        const text = composerText.trim();
        if (!text)
            return;
        onSend(text);
        setComposerText("");
        textareaRef.current?.focus();
    }
    return (_jsxs("div", { style: {
            display: "flex",
            gap: 8,
            padding: "8px 12px",
            borderTop: "1px solid var(--divider)",
            background: "rgba(255,255,255,0.05)",
            alignItems: "flex-end",
        }, children: [_jsx("textarea", { ref: textareaRef, value: composerText, onChange: (e) => setComposerText(e.target.value), onKeyDown: handleKeyDown, placeholder: "\u8F93\u5165\u6D88\u606F\u2026 (Enter \u53D1\u9001\uFF0CShift+Enter \u6362\u884C)", rows: 1, style: {
                    flex: 1,
                    resize: "none",
                    border: "1px solid rgba(0,0,0,0.1)",
                    borderRadius: 8,
                    padding: "8px 12px",
                    fontSize: 14,
                    background: "rgba(255,255,255,0.6)",
                    outline: "none",
                    minHeight: 36,
                    maxHeight: 120,
                    overflowY: "auto",
                    lineHeight: 1.5,
                    fontFamily: "var(--font-family-zh)",
                } }), _jsx(GlassButton, { variant: "primary", onClick: submit, disabled: !composerText.trim(), style: { padding: "8px 18px", flexShrink: 0 }, children: "\u53D1\u9001" })] }));
}
