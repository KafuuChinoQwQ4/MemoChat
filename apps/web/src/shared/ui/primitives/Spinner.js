import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
export function Spinner({ size = 24, color = "var(--color-brand-green)" }) {
    return (_jsxs("svg", { width: size, height: size, viewBox: "0 0 24 24", fill: "none", style: { animation: "mc-spin 0.8s linear infinite" }, "aria-label": "\u52A0\u8F7D\u4E2D", children: [_jsx("style", { children: `@keyframes mc-spin { to { transform: rotate(360deg) } }` }), _jsx("circle", { cx: "12", cy: "12", r: "9", stroke: color, strokeWidth: "2.5", strokeLinecap: "round", strokeDasharray: "40 20" })] }));
}
