/**
 * ThreeColumnShell — CSS Grid three-column app shell.
 * Columns: 72px icon sidebar | 250px list panel | 1fr content area.
 * Mirrors ShellViewModel::Page layout from the desktop QML shell.
 */
import type { ReactNode } from "react"
import styles from "./ThreeColumnShell.module.css"

export interface ThreeColumnShellProps {
  sidebar: ReactNode
  listPanel: ReactNode
  content: ReactNode
}

export function ThreeColumnShell({ sidebar, listPanel, content }: ThreeColumnShellProps) {
  return (
    <div className={styles.shell}>
      {/* Icon sidebar — 72px */}
      <nav className={styles.sidebar}>
        {sidebar}
      </nav>

      {/* List panel */}
      <aside className={styles.listPanel}>
        {listPanel}
      </aside>

      {/* Main content */}
      <main className={styles.content}>
        {content}
      </main>
    </div>
  )
}
