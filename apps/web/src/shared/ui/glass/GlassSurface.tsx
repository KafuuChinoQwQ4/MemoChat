/**
 * GlassSurface — base glass panel component.
 * Mirrors qml/components/GlassSurface.qml: backdrop-filter + sheen + glow.
 * Only consumes CSS token variables — no literal rgba values.
 */
import type { CSSProperties, HTMLAttributes, ReactNode } from "react"
import { cn } from "@/shared/lib/classnames"
import styles from "./glass.module.css"

export interface GlassSurfaceProps extends HTMLAttributes<HTMLDivElement> {
  children?: ReactNode
  elevated?: boolean
  /** Override border-radius (defaults to --glass-radius) */
  radius?: number | string
  className?: string
  style?: CSSProperties
}

export function GlassSurface({
  children,
  elevated = false,
  radius,
  className,
  style,
  ...rest
}: GlassSurfaceProps) {
  return (
    <div
      className={cn(styles.surface, elevated && styles.elevated, className)}
      style={{ ...(radius !== undefined ? { "--glass-radius": typeof radius === "number" ? `${radius}px` : radius } as CSSProperties : {}), ...style }}
      {...rest}
    >
      {children}
    </div>
  )
}
