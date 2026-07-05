/** ContactShellContent — friend list + apply list view */
import { useMemo } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"

export function ContactShellContent() {
  const friendsMap = useEntityStore((s) => s.friends)
  const friends = useMemo(() => Array.from(friendsMap.values()), [friendsMap])

  return (
    <div style={{ display: "flex", height: "100%" }}>
      {/* Friend list sidebar */}
      <GlassScrollArea style={{ width: 250, borderRight: "1px solid var(--divider)", flexShrink: 0 }}>
        <div style={{ padding: "12px 12px 8px", fontWeight: 600, fontSize: 15 }}>联系人</div>
        {friends.map((f) => (
          <button
            key={f.uid}
            style={{
              display: "flex",
              alignItems: "center",
              gap: 10,
              padding: "8px 12px",
              border: "none",
              background: "transparent",
              cursor: "pointer",
              width: "100%",
              textAlign: "left",
              borderRadius: 8,
            }}
          >
            <Avatar src={f.icon} name={f.name} size={38} />
            <div>
              <div style={{ fontWeight: 500, fontSize: 14 }}>{f.name}</div>
              <div style={{ fontSize: 12, color: "var(--text-secondary)" }}>{f.email}</div>
            </div>
          </button>
        ))}
        {friends.length === 0 && (
          <div style={{ textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }}>暂无好友</div>
        )}
      </GlassScrollArea>

      {/* Detail pane */}
      <GlassSurface style={{ flex: 1, height: "100%", display: "flex", alignItems: "center", justifyContent: "center", color: "var(--text-disabled)", fontSize: 14 }}>
        选择联系人查看详情
      </GlassSurface>
    </div>
  )
}
