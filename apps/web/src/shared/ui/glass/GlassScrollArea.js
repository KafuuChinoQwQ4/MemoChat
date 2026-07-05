import { jsx as _jsx } from "react/jsx-runtime";
import { cn } from "@/shared/lib/classnames";
import styles from "./glass.module.css";
export function GlassScrollArea({ children, className, ...rest }) {
    return (_jsx("div", { className: cn(styles.scrollArea, className), ...rest, children: children }));
}
