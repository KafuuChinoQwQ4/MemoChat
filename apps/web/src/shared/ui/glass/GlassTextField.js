import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
import { cn } from "@/shared/lib/classnames";
import styles from "./glass.module.css";
export function GlassTextField({ label, className, ...rest }) {
    return (_jsxs("div", { className: cn(styles.textField, className), children: [label && (_jsx("label", { style: { fontSize: 12, color: "var(--text-secondary)", userSelect: "none" }, children: label })), _jsx("input", { className: styles.textFieldInput, ...rest })] }));
}
