import { jsx as _jsx } from "react/jsx-runtime";
import { cn } from "@/shared/lib/classnames";
import styles from "./glass.module.css";
export function GlassButton({ children, variant = "default", className, ...rest }) {
    return (_jsx("button", { className: cn(styles.btn, variant === "primary" && styles.btnPrimary, variant === "danger" && styles.btnDanger, className), ...rest, children: children }));
}
