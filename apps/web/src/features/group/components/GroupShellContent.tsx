/** Group feature shell content */
import { useEffect, useMemo, useState } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { useSessionStore } from "@/core/session/sessionStore"
import { ReqId } from "@/core/network/opcodes/reqIds"
import {
  displayNameWithoutInternalId,
  groupPublicIdText,
  normalizePublicUserId,
  publicUserIdText,
} from "@/core/entities/displayIds"
import type { Friend, Group } from "@/core/entities/entityTypes"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"

const USER_ID_PATTERN = /^u[1-9][0-9]{8}$/

function roleLabel(role: string | undefined): string {
  if (role === "owner") return "群主"
  if (role === "admin") return "管理员"
  return "成员"
}

function groupDisplayName(group: Group): string {
  return displayNameWithoutInternalId(group.name, group.groupCode, group.groupId, "未命名群聊")
}

function friendDisplayName(friend: Friend): string {
  return displayNameWithoutInternalId(friend.name, friend.userId, friend.uid, "未知用户")
}

export function GroupShellContent() {
  const groupsMap = useEntityStore((s) => s.groups)
  const friendsMap = useEntityStore((s) => s.friends)
  const uid = useSessionStore((s) => s.uid) ?? 0
  const groups = useMemo(() => Array.from(groupsMap.values()), [groupsMap])
  const friends = useMemo(() => Array.from(friendsMap.values()), [friendsMap])
  const [selectedGroupId, setSelectedGroupId] = useState<number | null>(null)
  const [createOpen, setCreateOpen] = useState(false)
  const [groupName, setGroupName] = useState("")
  const [manualMembers, setManualMembers] = useState("")
  const [selectedUserIds, setSelectedUserIds] = useState<Set<string>>(() => new Set())
  const [statusText, setStatusText] = useState("")
  const selectedGroup = groups.find((g) => g.groupId === selectedGroupId) ?? null
  const gateway = useMemo(() => getGateway(), [])

  useEffect(() => {
    if (groups.length === 0) {
      setSelectedGroupId(null)
      return
    }
    if (selectedGroupId === null || !groups.some((g) => g.groupId === selectedGroupId)) {
      setSelectedGroupId(groups[0]?.groupId ?? null)
    }
  }, [groups, selectedGroupId])

  useEffect(() => {
    return gateway.dispatcher.subscribe(ReqId.ID_CREATE_GROUP_RSP, (frame) => {
      try {
        const data = JSON.parse(frame.payload) as { error?: number; group_code?: string; groupid?: number; name?: string }
        if (data.error === 0) {
          const groupIdText = groupPublicIdText(data.group_code)
          setStatusText(data.group_code ? `群聊已创建：${data.name || groupIdText}` : "群聊已创建")
          setCreateOpen(false)
          setGroupName("")
          setManualMembers("")
          setSelectedUserIds(new Set())
          if (uid > 0) {
            gateway.chatTransport.send(ReqId.ID_GET_GROUP_LIST_REQ, JSON.stringify({ fromuid: uid }))
          }
          return
        }
        setStatusText(`创建失败，错误码 ${data.error ?? "unknown"}`)
      } catch {
        setStatusText("创建失败，服务响应格式错误")
      }
    })
  }, [gateway, uid])

  function friendUserId(friendUid: number): string {
    const friend = friendsMap.get(friendUid)
    return publicUserIdText(friend?.userId)
  }

  function toggleUserId(userId: string) {
    if (!userId) return
    setSelectedUserIds((current) => {
      const next = new Set(current)
      if (next.has(userId)) {
        next.delete(userId)
      } else {
        next.add(userId)
      }
      return next
    })
  }

  function collectMemberUserIds(): string[] | null {
    const ids = new Set<string>()
    for (const id of selectedUserIds) {
      if (id) ids.add(id)
    }
    for (const part of manualMembers.split(/[\s,，]+/)) {
      const value = part.trim()
      if (value) ids.add(value)
    }
    for (const id of ids) {
      if (!USER_ID_PATTERN.test(id)) {
        setStatusText(`成员ID格式非法：${id}`)
        return null
      }
    }
    return Array.from(ids)
  }

  function handleCreateGroup() {
    const name = groupName.trim()
    if (uid <= 0) {
      setStatusText("登录状态无效，请重新登录")
      return
    }
    if (name.length === 0 || name.length > 64) {
      setStatusText("群名称长度需在1-64之间")
      return
    }
    const memberUserIds = collectMemberUserIds()
    if (memberUserIds === null) return
    gateway.chatTransport.send(ReqId.ID_CREATE_GROUP_REQ, JSON.stringify({
      fromuid: uid,
      name,
      member_limit: 200,
      member_user_ids: memberUserIds,
    }))
    setStatusText("正在创建群聊...")
  }

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
        <div style={{ padding: "6px 10px 12px", display: "flex", alignItems: "center", justifyContent: "space-between", gap: 8 }}>
          <div style={{ fontWeight: 700, fontSize: 16 }}>群组</div>
          <GlassButton
            type="button"
            onClick={() => setCreateOpen((value) => !value)}
            style={{ padding: "5px 9px", fontSize: 12 }}
          >
            建群
          </GlassButton>
        </div>
        {createOpen ? (
          <GlassSurface style={{ padding: 10, margin: "0 0 10px", borderRadius: 12 }}>
            <div style={{ display: "grid", gap: 8 }}>
              <GlassTextField
                value={groupName}
                onChange={(event) => setGroupName(event.target.value)}
                placeholder="群名称（1-64）"
                aria-label="群名称"
              />
              <div style={{ fontSize: 12, color: "var(--text-secondary)", fontWeight: 700 }}>选择好友</div>
              <div style={{ display: "grid", gap: 6, maxHeight: 176, overflowY: "auto", paddingRight: 2 }}>
                {friends.map((friend) => {
                  const userId = normalizePublicUserId(friend.userId)
                  const name = friendDisplayName(friend)
                  const checked = selectedUserIds.has(userId)
                  return (
                    <label
                      key={friend.uid}
                      style={{
                        display: "flex",
                        alignItems: "center",
                        gap: 8,
                        padding: "7px 8px",
                        borderRadius: 9,
                        background: checked ? "var(--tint-selected)" : "rgba(255,255,255,0.08)",
                        border: "1px solid var(--divider)",
                        cursor: userId ? "pointer" : "not-allowed",
                      }}
                    >
                      <input
                        type="checkbox"
                        checked={checked}
                        disabled={!userId}
                        onChange={() => toggleUserId(userId)}
                      />
                      <Avatar src={friend.icon} name={name} size={28} />
                      <span style={{ minWidth: 0 }}>
                        <span style={{ display: "block", fontSize: 12, fontWeight: 700, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                          {name}
                        </span>
                        <span style={{ display: "block", fontSize: 11, color: "var(--text-disabled)" }}>
                          {friendUserId(friend.uid) || "未分配用户ID"}
                        </span>
                      </span>
                    </label>
                  )
                })}
                {friends.length === 0 ? (
                  <div style={{ fontSize: 12, color: "var(--text-disabled)", textAlign: "center", padding: 10 }}>
                    暂无好友，可手动填写用户ID
                  </div>
                ) : null}
              </div>
              <textarea
                value={manualMembers}
                onChange={(event) => setManualMembers(event.target.value)}
                placeholder="手动补充用户ID，逗号或空格分隔"
                aria-label="手动群成员用户ID"
                style={{
                  minHeight: 58,
                  resize: "vertical",
                  borderRadius: 10,
                  border: "1px solid var(--divider)",
                  background: "rgba(255,255,255,0.10)",
                  color: "var(--text-primary)",
                  padding: "8px 10px",
                  font: "inherit",
                  fontSize: 13,
                  outline: "none",
                }}
              />
              <GlassButton type="button" variant="primary" onClick={handleCreateGroup}>
                创建群聊
              </GlassButton>
            </div>
          </GlassSurface>
        ) : null}
        {statusText ? (
          <div style={{ padding: "0 10px 10px", fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.45 }}>
            {statusText}
          </div>
        ) : null}
        {groups.map((g) => {
          const name = groupDisplayName(g)
          return (
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
              <Avatar src={g.icon} name={name} size={38} />
              <div style={{ minWidth: 0 }}>
                <div style={{ fontWeight: 600, fontSize: 14, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{name}</div>
                {g.memberCount && (
                  <div style={{ fontSize: 12, color: "var(--text-secondary)" }}>{g.memberCount} 人</div>
                )}
              </div>
            </button>
          )
        })}
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
                  name={groupDisplayName(selectedGroup)}
                  size={64}
                  style={{ boxShadow: "0 10px 30px rgba(0,0,0,0.10)" }}
                />
                <div style={{ minWidth: 0 }}>
                  <div style={{ fontSize: 22, fontWeight: 700, lineHeight: 1.2 }}>{groupDisplayName(selectedGroup)}</div>
                  <div style={{ marginTop: 4, fontSize: 13, color: "var(--text-secondary)" }}>
                    {(selectedGroup.memberCount ?? 0) > 0 ? `${selectedGroup.memberCount} 人` : "成员数未知"}
                  </div>
                  <div style={{ marginTop: 4, fontSize: 12, color: "var(--text-disabled)" }}>
                    群号: {groupPublicIdText(selectedGroup.groupCode)}
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
