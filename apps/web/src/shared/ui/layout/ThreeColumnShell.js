import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
export function ThreeColumnShell({ sidebar, listPanel, content }) {
    return (_jsxs("div", { style: {
            display: "grid",
            gridTemplateColumns: "var(--sidebar-w) var(--list-panel-w) 1fr",
            height: "100%",
            background: "var(--surface-bg)",
            overflow: "hidden",
        }, children: [_jsx("nav", { style: {
                    display: "flex",
                    flexDirection: "column",
                    alignItems: "center",
                    padding: "8px 0",
                    gap: 4,
                    background: "linear-gradient(180deg, rgba(255,255,255,0.075), rgba(255,255,255,0.022)), var(--sidebar-bg)",
                    backdropFilter: "blur(24px) saturate(1.6)",
                    WebkitBackdropFilter: "blur(24px) saturate(1.6)",
                    borderRight: "1px solid var(--sidebar-border)",
                    boxShadow: "inset -1px 0 0 rgba(255,255,255,0.10)",
                }, children: sidebar }), _jsx("aside", { style: {
                    display: "flex",
                    flexDirection: "column",
                    overflow: "hidden",
                    background: "var(--list-panel-bg)",
                    backdropFilter: "blur(16px) saturate(1.4)",
                    WebkitBackdropFilter: "blur(16px) saturate(1.4)",
                    borderRight: "1px solid var(--list-panel-border)",
                }, children: listPanel }), _jsx("main", { style: { display: "flex", flexDirection: "column", overflow: "hidden" }, children: content })] }));
}
