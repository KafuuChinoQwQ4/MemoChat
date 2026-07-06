/** Group feature shell content */
import { useEffect, useMemo, useState } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"

function roleLabel(role: string | undefined): string {
  if (role === "owner") return "群主"
  if (role === "admin") return "管理员"
  return "成员"
}

export function GroupShellContent() {
  const groupsMap = useEntityStore((s) => s.groups)
  const groups = useMemo(() => Array.from(groupsMap.values()), [groupsMap])
  const [selectedGroupId, setSelectedGroupId] = useState<number | null>(null)
  const selectedGroup = groups.find((g) => g.groupId === selectedGroupId) ?? null

  useEffect(() => {
    if (groups.length === 0) {
      setSelectedGroupId(null)
      return
    }
    if (selectedGroupId === null || !groups.some((g) => g.groupId === selectedGroupId)) {
      setSelectedGroupId(groups[0]?.groupId ?? null)
    }
  }, [groups, selectedGroupId])

  return (
    <div
      style={{
        display: "grid",
        gridTemplateColumns: "280px minmax(0, 1fr)",
        height: "100%",
        width: "100%",
        minWidth: 0,
        overflow: "hidden",
      }}
    >
      <GlassScrollArea style={{ borderRight: "1px solid var(--divider)", padding: "10px 8px" }}>
        <div style={{ padding: "6px 10px 12px", fontWeight: 700, fontSize: 16 }}>群组</div>
        {groups.map((g) => (
          <button
            key={g.groupId}
            onClick={() => setSelectedGroupId(g.groupId)}
            style={{
              display: "flex", alignItems: "center", gap: 10,
              padding: "10px 12px", border: "none",
              background: g.groupId === selectedGroupId ? "var(--tint-selected)" : "transparent",
              cursor: "pointer", width: "100%", textAlign: "left", borderRadius: 8,
              color: "var(--text-primary)",
            }}
          >
            <Avatar src={g.icon} name={g.name} size={38} />
            <div style={{ minWidth: 0 }}>
              <div style={{ fontWeight: 600, fontSize: 14, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{g.name}</div>
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

      <div style={{ minWidth: 0, padding: 16, overflow: "hidden" }}>
        <GlassSurface
          elevated
          style={{
            height: "100%",
            padding: 28,
            display: "flex",
            flexDirection: "column",
            color: "var(--text-primary)",
          }}
        >
          {selectedGroup ? (
            <>
              <div style={{ display: "flex", alignItems: "center", gap: 16 }}>
                <Avatar
                  src={selectedGroup.icon}
                  name={selectedGroup.name}
                  size={64}
                  style={{ boxShadow: "0 10px 30px rgba(0,0,0,0.10)" }}
                />
                <div style={{ minWidth: 0 }}>
                  <div style={{ fontSize: 22, fontWeight: 700, lineHeight: 1.2 }}>{selectedGroup.name}</div>
                  <div style={{ marginTop: 4, fontSize: 13, color: "var(--text-secondary)" }}>
                    {(selectedGroup.memberCount ?? 0) > 0 ? `${selectedGroup.memberCount} 人` : "成员数未知"}
                  </div>
                  <div style={{ marginTop: 4, fontSize: 12, color: "var(--text-disabled)" }}>
                    群号: {selectedGroup.groupId}
                  </div>
                </div>
              </div>
              <div style={{ height: 1, background: "var(--divider)", margin: "24px 0" }} />
              <div style={{ display: "grid", gap: 12, maxWidth: 560 }}>
                <div>
                  <div style={{ fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }}>我的身份</div>
                  <div style={{ fontSize: 14 }}>{roleLabel(selectedGroup.role)}</div>
                </div>
                <div>
                  <div style={{ fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }}>群公告</div>
                  <div style={{ fontSize: 14, lineHeight: 1.6, whiteSpace: "pre-wrap", overflowWrap: "anywhere" }}>
                    {selectedGroup.announcement?.trim() || "暂无公告"}
                  </div>
                </div>
              </div>
            </>
          ) : (
            <div style={{ margin: "auto", color: "var(--text-disabled)", fontSize: 14 }}>
              选择群组查看详情
            </div>
          )}
        </GlassSurface>
      </div>
    </div>
  )
}
