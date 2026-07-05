import { jsx as _jsx } from "react/jsx-runtime";
export function Badge({ count, dot }) {
    if (!dot && (!count || count <= 0))
        return null;
    return (_jsx("span", { style: {
            display: "inline-flex",
            alignItems: "center",
            justifyContent: "center",
            minWidth: dot ? 8 : 16,
            height: dot ? 8 : 16,
            borderRadius: 8,
            padding: dot ? 0 : "0 4px",
            background: "var(--color-badge)",
            color: "#fff",
            fontSize: 11,
            fontWeight: 700,
            lineHeight: 1,
        }, children: !dot && (count > 99 ? "99+" : count) }));
}
