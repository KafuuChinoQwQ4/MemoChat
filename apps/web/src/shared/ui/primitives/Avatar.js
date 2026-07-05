import { jsx as _jsx } from "react/jsx-runtime";
/** Avatar — displays user avatar with fallback initials */
import { useMediaUrl } from "@/shared/hooks/useMediaUrl";
import { cn } from "@/shared/lib/classnames";
export function Avatar({ src, name, size = 40, className, onClick }) {
    const url = useMediaUrl(src);
    const initials = name ? name.slice(0, 2).toUpperCase() : "?";
    return (_jsx("div", { className: cn("avatar", className), onClick: onClick, style: {
            width: size,
            height: size,
            borderRadius: "50%",
            overflow: "hidden",
            flexShrink: 0,
            cursor: onClick ? "pointer" : undefined,
            backgroundColor: "var(--tint-hover)",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            fontSize: size * 0.4,
            fontWeight: 600,
            color: "var(--text-secondary)",
        }, children: url ? (_jsx("img", { src: url, alt: name, style: { width: "100%", height: "100%", objectFit: "cover" }, onError: (e) => { e.currentTarget.style.display = "none"; } })) : (initials) }));
}
