/** GlassTextField — input with glass styling and focus ring */
import type { InputHTMLAttributes } from "react"
import { cn } from "@/shared/lib/classnames"
import styles from "./glass.module.css"

export interface GlassTextFieldProps extends InputHTMLAttributes<HTMLInputElement> {
  label?: string
  className?: string
}

export function GlassTextField({ label, className, ...rest }: GlassTextFieldProps) {
  return (
    <div className={cn(styles.textField, className)}>
      {label && (
        <label style={{ fontSize: 12, color: "var(--text-secondary)", userSelect: "none" }}>
          {label}
        </label>
      )}
      <input className={styles.textFieldInput} {...rest} />
    </div>
  )
}
