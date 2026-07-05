/**
 * ThreeColumnShell — CSS Grid three-column app shell.
 * Columns: 72px icon sidebar | 250px list panel | 1fr content area.
 * Mirrors ShellViewModel::Page layout from the desktop QML shell.
 */
import type { ReactNode } from "react"

export interface ThreeColumnShellProps {
  sidebar: ReactNode
  listPanel: ReactNode
  content: ReactNode
}

export function ThreeColumnShell({ sidebar, listPanel, content }: ThreeColumnShellProps) {
  return (
    <div
      style={{
        display: "grid",
        gridTemplateColumns: "var(--sidebar-w) var(--list-panel-w) 1fr",
        height: "100%",
        gap: "var(--col-gap)",
        background: "var(--surface-bg)",
        overflow: "hidden",
      }}
    >
      {/* Icon sidebar — 72px */}
      <nav
        style={{
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          padding: "8px 0",
          gap: 4,
          background: "rgba(255,255,255,0.08)",
          borderRight: "1px solid var(--divider)",
        }}
      >
        {sidebar}
      </nav>

      {/* List panel — ~250px */}
      <aside
        style={{
          display: "flex",
          flexDirection: "column",
          overflow: "hidden",
          background: "rgba(255,255,255,0.05)",
          borderRight: "1px solid var(--divider)",
        }}
      >
        {listPanel}
      </aside>

      {/* Main content — flexible */}
      <main style={{ display: "flex", flexDirection: "column", overflow: "hidden" }}>
        {content}
      </main>
    </div>
  )
}
