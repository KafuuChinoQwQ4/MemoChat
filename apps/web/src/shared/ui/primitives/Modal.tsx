/**
 * Modal — portal-based overlay. Open/close state lives in the feature store
 * (never inside Modal itself), mirroring ChatModalLayer.qml pattern.
 */
import { createPortal } from "react-dom"
import type { ReactNode } from "react"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassButton } from "@/shared/ui/glass/GlassButton"

export interface ModalProps {
  open: boolean
  onClose: () => void
  title?: string
  children?: ReactNode
  width?: number
}

export function Modal({ open, onClose, title, children, width = 420 }: ModalProps) {
  if (!open) return null
  return createPortal(
    <div
      style={{
        position: "fixed",
        inset: 0,
        zIndex: 1000,
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        background: "rgba(0,0,0,0.35)",
        backdropFilter: "blur(4px)",
      }}
      onClick={(e) => { if (e.target === e.currentTarget) onClose() }}
    >
      <GlassSurface
        elevated
        style={{
          width,
          maxWidth: "90vw",
          maxHeight: "85vh",
          display: "flex",
          flexDirection: "column",
          borderRadius: "var(--panel-radius)",
          overflow: "hidden",
        }}
      >
        {title && (
          <div style={{ padding: "14px 16px 0", display: "flex", alignItems: "center", justifyContent: "space-between" }}>
            <span style={{ fontWeight: 600, fontSize: 15 }}>{title}</span>
            <GlassButton
              onClick={onClose}
              aria-label="关闭"
              style={{ padding: "4px 10px", fontSize: 18, lineHeight: 1 }}
            >
              ×
            </GlassButton>
          </div>
        )}
        <div style={{ flex: 1, overflow: "auto", padding: 16 }}>{children}</div>
      </GlassSurface>
    </div>,
    document.body,
  )
}
