/** Lightweight classnames helper — avoids adding clsx dep at call sites */
export function cn(...args) {
    return args.filter(Boolean).join(" ");
}
