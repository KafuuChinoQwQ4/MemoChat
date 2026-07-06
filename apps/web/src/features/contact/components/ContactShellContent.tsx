/** ContactShellContent — friend list + apply list view */
import { useEffect, useMemo, useState } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"

export function ContactShellContent() {
  const friendsMap = useEntityStore((s) => s.friends)
  const friends = useMemo(() => Array.from(friendsMap.values()), [friendsMap])
  const [selectedUid, setSelectedUid] = useState<number | null>(null)
  const selectedFriend = friends.find((f) => f.uid === selectedUid) ?? null

  useEffect(() => {
    if (friends.length === 0) {
      setSelectedUid(null)
      return
    }
    if (selectedUid === null || !friends.some((f) => f.uid === selectedUid)) {
      setSelectedUid(friends[0]?.uid ?? null)
    }
  }, [friends, selectedUid])

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
      {/* Friend list sidebar */}
      <GlassScrollArea style={{ borderRight: "1px solid var(--divider)", padding: "10px 8px" }}>
        <div style={{ padding: "6px 10px 12px", fontWeight: 700, fontSize: 16 }}>联系人</div>
        {friends.map((f) => (
          <button
            key={f.uid}
            onClick={() => setSelectedUid(f.uid)}
            style={{
              display: "flex",
              alignItems: "center",
              gap: 10,
              padding: "10px 12px",
              border: "none",
              background: f.uid === selectedUid ? "var(--tint-selected)" : "transparent",
              cursor: "pointer",
              width: "100%",
              textAlign: "left",
              borderRadius: 10,
              color: "var(--text-primary)",
              transition: "background 140ms ease, transform 120ms ease",
            }}
          >
            <Avatar src={f.icon} name={f.name} size={38} />
            <div style={{ minWidth: 0 }}>
              <div style={{ fontWeight: 600, fontSize: 14, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{f.name}</div>
              <div style={{ fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{f.email}</div>
            </div>
          </button>
        ))}
        {friends.length === 0 && (
          <div style={{ textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }}>暂无好友</div>
        )}
      </GlassScrollArea>

      {/* Detail pane */}
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
          {selectedFriend ? (
            <>
              <div style={{ display: "flex", alignItems: "center", gap: 16 }}>
                <Avatar
                  src={selectedFriend.icon}
                  name={selectedFriend.name}
                  size={64}
                  style={{ boxShadow: "0 10px 30px rgba(0,0,0,0.10)" }}
                />
                <div style={{ minWidth: 0 }}>
                  <div style={{ fontSize: 22, fontWeight: 700, lineHeight: 1.2 }}>{selectedFriend.name}</div>
                  <div style={{ marginTop: 4, fontSize: 13, color: "var(--text-secondary)" }}>{selectedFriend.email}</div>
                  <div style={{ marginTop: 4, fontSize: 12, color: "var(--text-disabled)" }}>UID: {selectedFriend.uid}</div>
                </div>
              </div>
              <div style={{ height: 1, background: "var(--divider)", margin: "24px 0" }} />
              <div style={{ display: "grid", gap: 12, maxWidth: 520 }}>
                <div>
                  <div style={{ fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }}>备注</div>
                  <div style={{ fontSize: 14 }}>{selectedFriend.desc?.trim() || "暂无备注"}</div>
                </div>
                <div>
                  <div style={{ fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }}>账号状态</div>
                  <div style={{ fontSize: 14 }}>已添加为好友</div>
                </div>
              </div>
            </>
          ) : (
            <div style={{ margin: "auto", color: "var(--text-disabled)", fontSize: 14 }}>
              选择联系人查看详情
            </div>
          )}
        </GlassSurface>
      </div>
    </div>
  )
}
