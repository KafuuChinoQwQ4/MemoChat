/** GlassScrollArea — scrollable container with glass-aware scrollbar */
import type { HTMLAttributes, ReactNode } from "react"
import { cn } from "@/shared/lib/classnames"
import styles from "./glass.module.css"

export interface GlassScrollAreaProps extends HTMLAttributes<HTMLDivElement> {
  children?: ReactNode
  className?: string
}

export function GlassScrollArea({ children, className, ...rest }: GlassScrollAreaProps) {
  return (
    <div className={cn(styles.scrollArea, className)} {...rest}>
      {children}
    </div>
  )
}
