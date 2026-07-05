/** GlassButton — token-driven interactive button */
import type { ButtonHTMLAttributes, ReactNode } from "react"
import { cn } from "@/shared/lib/classnames"
import styles from "./glass.module.css"

export interface GlassButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  children?: ReactNode
  variant?: "default" | "primary" | "danger"
  className?: string
}

export function GlassButton({ children, variant = "default", className, ...rest }: GlassButtonProps) {
  return (
    <button
      className={cn(
        styles.btn,
        variant === "primary" && styles.btnPrimary,
        variant === "danger" && styles.btnDanger,
        className,
      )}
      {...rest}
    >
      {children}
    </button>
  )
}
