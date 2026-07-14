/** ContactShellContent — friend list + apply list view */
import { useEffect, useMemo, useState } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { useSessionStore } from "@/core/session/sessionStore"
import { ReqId } from "@/core/network/opcodes/reqIds"
import { displayNameWithoutInternalId, publicUserIdText } from "@/core/entities/displayIds"
import type { Friend } from "@/core/entities/entityTypes"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { groupContactsByInitial } from "@/shared/lib/contactListGrouping"

const USER_ID_PATTERN = /^u[1-9][0-9]{8}$/

interface SearchUserResult {
  uid: number
  name: string
  userId: string
  icon: string
  desc: string
}

function parseSearchUser(payload: string): SearchUserResult | null {
  try {
    const data = JSON.parse(payload) as {
      error?: number
      uid?: number
      name?: string
      nick?: string
      user_id?: string
      icon?: string
      desc?: string
    }
    if (data.error !== 0 || !data.uid || data.uid <= 0) return null
    return {
      uid: data.uid,
      name: data.nick?.trim() || data.name?.trim() || data.user_id?.trim() || "未知用户",
      userId: data.user_id?.trim() || "",
      icon: data.icon || "",
      desc: data.desc || "",
    }
  } catch {
    return null
  }
}

function friendDisplayName(friend: Friend): string {
  return displayNameWithoutInternalId(friend.name, friend.userId, friend.uid, "未知用户")
}

function ContactInitialHeader({ initial }: { initial: string }) {
  return (
    <div
      role="heading"
      aria-level={2}
      style={{
        position: "sticky",
        top: 0,
        zIndex: 2,
        padding: "7px 10px 5px",
        background: "color-mix(in srgb, var(--sidebar-bg) 88%, transparent)",
        backdropFilter: "blur(12px)",
        WebkitBackdropFilter: "blur(12px)",
        color: "var(--text-secondary)",
        fontSize: 11,
        fontWeight: 700,
      }}
    >
      {initial}
    </div>
  )
}

export function ContactShellContent() {
  const friendsMap = useEntityStore((s) => s.friends)
  const applyList = useEntityStore((s) => s.applyList)
  const uid = useSessionStore((s) => s.uid) ?? 0
  const profile = useSessionStore((s) => s.profile)
  const friends = useMemo(() => Array.from(friendsMap.values()), [friendsMap])
  const groupedFriends = useMemo(
    () => groupContactsByInitial(friends, friendDisplayName),
    [friends],
  )
  const orderedFriends = useMemo(
    () => groupedFriends.flatMap((group) => group.items),
    [groupedFriends],
  )
  const pendingApplies = useMemo(
    () => applyList.filter((item) => item.status === "pending"),
    [applyList],
  )
  const [selectedUid, setSelectedUid] = useState<number | null>(null)
  const [searchUserId, setSearchUserId] = useState("")
  const [remark, setRemark] = useState("")
  const [searchResult, setSearchResult] = useState<SearchUserResult | null>(null)
  const [statusText, setStatusText] = useState("")
  const [searching, setSearching] = useState(false)
  const selectedFriend = friends.find((f) => f.uid === selectedUid) ?? null
  const gateway = useMemo(() => getGateway(), [])

  useEffect(() => {
    if (friends.length === 0) {
      setSelectedUid(null)
      return
    }
    if (selectedUid === null || !friends.some((f) => f.uid === selectedUid)) {
      setSelectedUid(orderedFriends[0]?.uid ?? null)
    }
  }, [friends, orderedFriends, selectedUid])

  useEffect(() => {
    const unsubs = [
      gateway.dispatcher.subscribe(ReqId.ID_SEARCH_USER_RSP, (frame) => {
        setSearching(false)
        const result = parseSearchUser(frame.payload)
        setSearchResult(result)
        setStatusText(result ? "已找到用户，可以发送好友申请" : "没有找到这个用户")
      }),
      gateway.dispatcher.subscribe(ReqId.ID_ADD_FRIEND_RSP, (frame) => {
        try {
          const data = JSON.parse(frame.payload) as { error?: number }
          setStatusText(data.error === 0 ? "好友申请已发送" : `申请失败，错误码 ${data.error ?? "unknown"}`)
        } catch {
          setStatusText("申请失败，服务响应格式错误")
        }
      }),
      gateway.dispatcher.subscribe(ReqId.ID_AUTH_FRIEND_RSP, (frame) => {
        try {
          const data = JSON.parse(frame.payload) as { error?: number }
          setStatusText(data.error === 0 ? "已同意好友申请" : `同意失败，错误码 ${data.error ?? "unknown"}`)
        } catch {
          setStatusText("同意失败，服务响应格式错误")
        }
      }),
    ]
    return () => unsubs.forEach((unsubscribe) => unsubscribe())
  }, [gateway])

  function handleSearch() {
    const userId = searchUserId.trim()
    setSearchResult(null)
    if (!USER_ID_PATTERN.test(userId)) {
      setStatusText("请输入有效用户ID，例如 u123456789")
      return
    }
    setSearching(true)
    setStatusText("正在搜索用户...")
    gateway.chatTransport.send(ReqId.ID_SEARCH_USER_REQ, JSON.stringify({ user_id: userId }))
  }

  function handleAddFriend() {
    if (uid <= 0) {
      setStatusText("登录状态无效，请重新登录")
      return
    }
    if (!searchResult) {
      setStatusText("请先搜索用户")
      return
    }
    if (searchResult.uid === uid) {
      setStatusText("不能添加自己")
      return
    }
    const displayName = profile?.name || profile?.userId || "未知用户"
    gateway.chatTransport.send(ReqId.ID_ADD_FRIEND_REQ, JSON.stringify({
      uid,
      applyname: displayName,
      bakname: remark.trim() || displayName,
      touid: searchResult.uid,
      labels: [],
    }))
    setStatusText("好友申请已提交，等待服务端确认")
  }

  function handleApprove(fromUid: number, fallbackName: string) {
    if (uid <= 0 || fromUid <= 0) {
      setStatusText("好友申请参数无效")
      return
    }
    gateway.chatTransport.send(ReqId.ID_AUTH_FRIEND_REQ, JSON.stringify({
      fromuid: uid,
      touid: fromUid,
      back: fallbackName,
      labels: [],
    }))
    setStatusText("正在同意好友申请...")
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
      {/* Friend list sidebar */}
      <GlassScrollArea style={{ borderRight: "1px solid var(--divider)", padding: "10px 8px" }}>
        <div style={{ padding: "6px 10px 12px", fontWeight: 700, fontSize: 16 }}>好友</div>
        <GlassSurface style={{ padding: 10, margin: "0 0 10px", borderRadius: 12 }}>
          <div style={{ fontSize: 13, fontWeight: 700, marginBottom: 8 }}>添加好友</div>
          <div style={{ display: "grid", gap: 8 }}>
            <GlassTextField
              value={searchUserId}
              onChange={(event) => setSearchUserId(event.target.value)}
              placeholder="用户ID，例如 u123456789"
              aria-label="搜索用户ID"
            />
            <GlassTextField
              value={remark}
              onChange={(event) => setRemark(event.target.value)}
              placeholder="验证信息/备注（可选）"
              aria-label="好友申请备注"
            />
            <GlassButton type="button" onClick={handleSearch} disabled={searching}>
              {searching ? "搜索中" : "搜索"}
            </GlassButton>
          </div>
          {searchResult ? (
            <div
              style={{
                display: "flex",
                alignItems: "center",
                gap: 9,
                marginTop: 10,
                padding: 8,
                borderRadius: 10,
                background: "rgba(255,255,255,0.10)",
                border: "1px solid var(--divider)",
              }}
            >
              <Avatar src={searchResult.icon} name={searchResult.name} size={34} />
              <div style={{ minWidth: 0, flex: 1 }}>
                <div style={{ fontSize: 13, fontWeight: 700, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                  {searchResult.name}
                </div>
                <div style={{ fontSize: 11, color: "var(--text-secondary)" }}>
                  {publicUserIdText(searchResult.userId)}
                </div>
              </div>
              <GlassButton type="button" onClick={handleAddFriend} style={{ padding: "5px 8px", fontSize: 12 }}>
                申请
              </GlassButton>
            </div>
          ) : null}
          {statusText ? (
            <div style={{ marginTop: 8, fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.45 }}>
              {statusText}
            </div>
          ) : null}
        </GlassSurface>

        {pendingApplies.length > 0 ? (
          <GlassSurface style={{ padding: 10, margin: "0 0 10px", borderRadius: 12 }}>
            <div style={{ fontSize: 13, fontWeight: 700, marginBottom: 8 }}>好友申请</div>
            <div style={{ display: "grid", gap: 8 }}>
              {pendingApplies.map((apply) => {
                const name = displayNameWithoutInternalId(
                  apply.nick || apply.name,
                  apply.userId,
                  apply.fromUid,
                  "未知用户",
                )
                return (
                  <div
                    key={`${apply.fromUid}-${apply.applyTime}`}
                    style={{
                      display: "flex",
                      gap: 8,
                      alignItems: "center",
                      minWidth: 0,
                    }}
                  >
                    <Avatar src={apply.icon} name={name} size={30} />
                    <div style={{ minWidth: 0, flex: 1 }}>
                      <div style={{ fontSize: 12, fontWeight: 700, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                        {name}
                      </div>
                      <div style={{ fontSize: 11, color: "var(--text-disabled)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                        {publicUserIdText(apply.userId)}
                      </div>
                    </div>
                    <GlassButton
                      type="button"
                      onClick={() => handleApprove(apply.fromUid, name)}
                      style={{ padding: "4px 7px", fontSize: 12 }}
                    >
                      同意
                    </GlassButton>
                  </div>
                )
              })}
            </div>
          </GlassSurface>
        ) : null}

        {groupedFriends.map((group) => (
          <section key={group.initial} aria-label={`${group.initial} 组`}>
            <ContactInitialHeader initial={group.initial} />
            {group.items.map((friend) => {
              const name = friendDisplayName(friend)
              return (
                <button
                  key={friend.uid}
                  onClick={() => setSelectedUid(friend.uid)}
                  style={{
                    display: "flex",
                    alignItems: "center",
                    gap: 10,
                    padding: "10px 12px",
                    border: "none",
                    background: friend.uid === selectedUid ? "var(--tint-selected)" : "transparent",
                    cursor: "pointer",
                    width: "100%",
                    textAlign: "left",
                    borderRadius: 10,
                    color: "var(--text-primary)",
                    transition: "background 140ms ease, transform 120ms ease",
                  }}
                >
                  <Avatar src={friend.icon} name={name} size={38} />
                  <div style={{ minWidth: 0 }}>
                    <div style={{ fontWeight: 600, fontSize: 14, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{name}</div>
                    <div style={{ fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                      {publicUserIdText(friend.userId)}
                    </div>
                  </div>
                </button>
              )
            })}
          </section>
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
                  name={friendDisplayName(selectedFriend)}
                  size={64}
                  style={{ boxShadow: "0 10px 30px rgba(0,0,0,0.10)" }}
                />
                <div style={{ minWidth: 0 }}>
                  <div style={{ fontSize: 22, fontWeight: 700, lineHeight: 1.2 }}>
                    {friendDisplayName(selectedFriend)}
                  </div>
                  <div style={{ marginTop: 4, fontSize: 13, color: "var(--text-secondary)" }}>
                    {publicUserIdText(selectedFriend.userId)}
                  </div>
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
