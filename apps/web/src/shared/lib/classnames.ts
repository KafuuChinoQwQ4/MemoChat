/** Lightweight classnames helper — avoids adding clsx dep at call sites */
export function cn(...args: (string | undefined | null | false)[]): string {
  return args.filter(Boolean).join(" ")
}
