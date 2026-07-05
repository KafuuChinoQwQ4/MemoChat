import { jsx as _jsx } from "react/jsx-runtime";
import { cn } from "@/shared/lib/classnames";
import styles from "./glass.module.css";
export function GlassSurface({ children, elevated = false, radius, className, style, ...rest }) {
    return (_jsx("div", { className: cn(styles.surface, elevated && styles.elevated, className), style: { ...(radius !== undefined ? { "--glass-radius": typeof radius === "number" ? `${radius}px` : radius } : {}), ...style }, ...rest, children: children }));
}
