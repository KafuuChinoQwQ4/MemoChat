import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/**
 * Modal — portal-based overlay. Open/close state lives in the feature store
 * (never inside Modal itself), mirroring ChatModalLayer.qml pattern.
 */
import { createPortal } from "react-dom";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
export function Modal({ open, onClose, title, children, width = 420 }) {
    if (!open)
        return null;
    return createPortal(_jsx("div", { style: {
            position: "fixed",
            inset: 0,
            zIndex: 1000,
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            background: "rgba(0,0,0,0.35)",
            backdropFilter: "blur(4px)",
        }, onClick: (e) => { if (e.target === e.currentTarget)
            onClose(); }, children: _jsxs(GlassSurface, { elevated: true, style: {
                width,
                maxWidth: "90vw",
                maxHeight: "85vh",
                display: "flex",
                flexDirection: "column",
                borderRadius: "var(--panel-radius)",
                overflow: "hidden",
            }, children: [title && (_jsxs("div", { style: { padding: "14px 16px 0", display: "flex", alignItems: "center", justifyContent: "space-between" }, children: [_jsx("span", { style: { fontWeight: 600, fontSize: 15 }, children: title }), _jsx(GlassButton, { onClick: onClose, "aria-label": "\u5173\u95ED", style: { padding: "4px 10px", fontSize: 18, lineHeight: 1 }, children: "\u00D7" })] })), _jsx("div", { style: { flex: 1, overflow: "auto", padding: 16 }, children: children })] }) }), document.body);
}
