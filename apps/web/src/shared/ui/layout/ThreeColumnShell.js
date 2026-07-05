import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
export function ThreeColumnShell({ sidebar, listPanel, content }) {
    return (_jsxs("div", { style: {
            display: "grid",
            gridTemplateColumns: "var(--sidebar-w) var(--list-panel-w) 1fr",
            height: "100%",
            gap: "var(--col-gap)",
            background: "var(--surface-bg)",
            overflow: "hidden",
        }, children: [_jsx("nav", { style: {
                    display: "flex",
                    flexDirection: "column",
                    alignItems: "center",
                    padding: "8px 0",
                    gap: 4,
                    background: "rgba(255,255,255,0.08)",
                    borderRight: "1px solid var(--divider)",
                }, children: sidebar }), _jsx("aside", { style: {
                    display: "flex",
                    flexDirection: "column",
                    overflow: "hidden",
                    background: "rgba(255,255,255,0.05)",
                    borderRight: "1px solid var(--divider)",
                }, children: listPanel }), _jsx("main", { style: { display: "flex", flexDirection: "column", overflow: "hidden" }, children: content })] }));
}
