/** Spinner — loading indicator */
export interface SpinnerProps {
  size?: number
  color?: string
}

export function Spinner({ size = 24, color = "var(--color-brand-green)" }: SpinnerProps) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      style={{ animation: "mc-spin 0.8s linear infinite" }}
      aria-label="加载中"
    >
      <style>{`@keyframes mc-spin { to { transform: rotate(360deg) } }`}</style>
      <circle cx="12" cy="12" r="9" stroke={color} strokeWidth="2.5" strokeLinecap="round"
        strokeDasharray="40 20" />
    </svg>
  )
}
