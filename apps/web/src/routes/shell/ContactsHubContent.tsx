import { useState } from "react"
import { ContactShellContent } from "@/features/contact/components/ContactShellContent"
import { GroupShellContent } from "@/features/group/components/GroupShellContent"

type ContactView = "friends" | "groups"

export function ContactsHubContent() {
  const [view, setView] = useState<ContactView>("friends")

  return (
    <div
      style={{
        display: "flex",
        flexDirection: "column",
        width: "100%",
        minWidth: 0,
        height: "100%",
        minHeight: 0,
      }}
    >
      <div
        style={{
          display: "flex",
          alignItems: "center",
          gap: 6,
          padding: "10px 16px 8px",
          borderBottom: "1px solid var(--divider)",
          background: "color-mix(in srgb, var(--sidebar-bg) 92%, transparent)",
        }}
      >
        <div style={{ fontSize: 17, fontWeight: 700, marginRight: 10 }}>联系人</div>
        <div role="tablist" aria-label="联系人类型" style={{ display: "flex", gap: 4 }}>
          {([
            ["friends", "好友"],
            ["groups", "群聊"],
          ] as const).map(([value, label]) => {
            const selected = view === value
            return (
              <button
                key={value}
                type="button"
                role="tab"
                aria-selected={selected}
                onClick={() => setView(value)}
                style={{
                  border: "none",
                  borderRadius: 8,
                  padding: "6px 12px",
                  background: selected ? "var(--tint-selected)" : "transparent",
                  color: selected ? "var(--text-primary)" : "var(--text-secondary)",
                  font: "inherit",
                  fontSize: 13,
                  fontWeight: selected ? 700 : 500,
                  cursor: "pointer",
                }}
              >
                {label}
              </button>
            )
          })}
        </div>
      </div>
      <div style={{ flex: 1, minHeight: 0, overflow: "hidden" }}>
        {view === "friends" ? <ContactShellContent /> : <GroupShellContent />}
      </div>
    </div>
  )
}
