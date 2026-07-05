import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/**
 * BootstrapGate — shows a loading spinner while post-login bootstrap runs.
 * Only blocks child render, not navigation itself.
 */
import { Outlet } from "react-router-dom";
import { useSessionStore } from "@/core/session/sessionStore";
import { Spinner } from "@/shared/ui/primitives/Spinner";
export function BootstrapGate() {
    const connState = useSessionStore((s) => s.connState);
    // While connecting or waiting for chat-login, show a spinner
    if (connState === "connecting" || connState === "chat_login") {
        return (_jsxs("div", { style: {
                height: "100%",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                gap: 12,
                color: "var(--text-secondary)",
                fontSize: 14,
            }, children: [_jsx(Spinner, { size: 28 }), _jsx("span", { children: "\u6B63\u5728\u8FDE\u63A5\u2026" })] }));
    }
    return _jsx(Outlet, {});
}
