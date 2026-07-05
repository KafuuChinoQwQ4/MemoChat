/** Group feature shell content */
import { useMemo } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"

export function GroupShellContent() {
  const groupsMap = useEntityStore((s) => s.groups)
  const groups = useMemo(() => Array.from(groupsMap.values()), [groupsMap])

  return (
    <div style={{ display: "flex", height: "100%" }}>
      <GlassScrollArea style={{ width: 250, borderRight: "1px solid var(--divider)", flexShrink: 0 }}>
        <div style={{ padding: "12px 12px 8px", fontWeight: 600, fontSize: 15 }}>群组</div>
        {groups.map((g) => (
          <button
            key={g.groupId}
            style={{
              display: "flex", alignItems: "center", gap: 10,
              padding: "8px 12px", border: "none", background: "transparent",
              cursor: "pointer", width: "100%", textAlign: "left", borderRadius: 8,
            }}
          >
            <Avatar src={g.icon} name={g.name} size={38} />
            <div>
              <div style={{ fontWeight: 500, fontSize: 14 }}>{g.name}</div>
              {g.memberCount && (
                <div style={{ fontSize: 12, color: "var(--text-secondary)" }}>{g.memberCount} 人</div>
              )}
            </div>
          </button>
        ))}
        {groups.length === 0 && (
          <div style={{ textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }}>暂无群组</div>
        )}
      </GlassScrollArea>

      <GlassSurface style={{ flex: 1, height: "100%", display: "flex", alignItems: "center", justifyContent: "center", color: "var(--text-disabled)", fontSize: 14 }}>
        选择群组查看详情
      </GlassSurface>
    </div>
  )
}
