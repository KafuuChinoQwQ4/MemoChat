/** GlassScrollArea — scrollable container with glass-aware scrollbar */
import { forwardRef, type HTMLAttributes, type ReactNode } from "react"
import { cn } from "@/shared/lib/classnames"
import styles from "./glass.module.css"

export interface GlassScrollAreaProps extends HTMLAttributes<HTMLDivElement> {
  children?: ReactNode
  className?: string
}

export const GlassScrollArea = forwardRef<HTMLDivElement, GlassScrollAreaProps>(
  function GlassScrollArea({ children, className, ...rest }, ref) {
    return (
      <div ref={ref} className={cn(styles.scrollArea, className)} {...rest}>
        {children}
      </div>
    )
  },
)
