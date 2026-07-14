/** GroupManagementPanel — desktop-aligned group management sheet */
import { useEffect, useMemo, useRef, useState } from "react"
import type { Friend, Group, GroupApplyEntry, GroupMemberEntry } from "@/core/entities/entityTypes"
import {
  displayNameWithoutInternalId,
  groupPublicIdText,
  normalizePublicUserId,
  publicUserIdText,
} from "@/core/entities/displayIds"
import {
  GROUP_DEFAULT_ADMIN_PERMISSION_BITS,
  GROUP_PERM_BAN_USERS,
  GROUP_PERM_CHANGE_INFO,
  GROUP_PERM_DELETE_MESSAGES,
  GROUP_PERM_INVITE_USERS,
  GROUP_PERM_MANAGE_ADMINS,
  GROUP_PERM_MANAGE_TOPICS,
  GROUP_PERM_PIN_MESSAGES,
} from "@/app/dispatch/chatListPayloads"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"

const USER_ID_PATTERN = /^u[1-9][0-9]{8}$/

export interface GroupManagementActions {
  inviteGroupMember(targetUserId: string, reason?: string): void
  kickGroupMember(targetUserId: string): void
  setGroupAdmin(targetUserId: string, isAdmin: boolean, permissionBits?: number): void
  muteGroupMember(targetUserId: string, muteSeconds: number): void
  reviewGroupApply(applyId: number, agree: boolean): void
  updateGroupAnnouncement(announcement: string): void
  updateGroupIconFromFile(file: File): Promise<void>
  quitGroup(): void
  dissolveGroup(): void
  refreshGroupList(): void
}

export interface GroupManagementPanelProps {
  group: Group
  friends: Friend[]
  members?: GroupMemberEntry[]
  pendingApplies?: GroupApplyEntry[]
  open: boolean
  statusText?: string
  statusError?: boolean
  busy?: boolean
  onClose: () => void
  actions: GroupManagementActions
}

function roleLabel(role: Group["role"] | undefined): string {
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

function availablePermissionOptions(group: Group): Array<{ label: string; bit: number; defaultChecked: boolean }> {
  const items: Array<{ label: string; bit: number; defaultChecked: boolean }> = []
  if (group.canChangeInfo) items.push({ label: "群资料", bit: GROUP_PERM_CHANGE_INFO, defaultChecked: true })
  if (group.canDeleteMessages) items.push({ label: "删消息", bit: GROUP_PERM_DELETE_MESSAGES, defaultChecked: true })
  if (group.canInviteUsers) items.push({ label: "邀成员", bit: GROUP_PERM_INVITE_USERS, defaultChecked: true })
  if (group.canManageAdmins) items.push({ label: "管理员", bit: GROUP_PERM_MANAGE_ADMINS, defaultChecked: false })
  if (group.canPinMessages) items.push({ label: "置顶", bit: GROUP_PERM_PIN_MESSAGES, defaultChecked: true })
  if (group.canBanUsers) items.push({ label: "禁言踢人", bit: GROUP_PERM_BAN_USERS, defaultChecked: true })
  if (group.canManageTopics) items.push({ label: "话题", bit: GROUP_PERM_MANAGE_TOPICS, defaultChecked: false })
  if (items.length === 0) {
    // Fallback to desktop default admin bits when flags missing but manage-admin is allowed.
    return [
      { label: "群资料", bit: GROUP_PERM_CHANGE_INFO, defaultChecked: true },
      { label: "删消息", bit: GROUP_PERM_DELETE_MESSAGES, defaultChecked: true },
      { label: "邀成员", bit: GROUP_PERM_INVITE_USERS, defaultChecked: true },
      { label: "置顶", bit: GROUP_PERM_PIN_MESSAGES, defaultChecked: true },
      { label: "禁言踢人", bit: GROUP_PERM_BAN_USERS, defaultChecked: true },
    ]
  }
  return items
}

export function GroupManagementPanel({
  group,
  friends,
  members = [],
  pendingApplies = [],
  open,
  statusText = "",
  statusError = false,
  busy = false,
  onClose,
  actions,
}: GroupManagementPanelProps) {
  const fileInputRef = useRef<HTMLInputElement>(null)
  const [targetUserId, setTargetUserId] = useState("")
  const [reason, setReason] = useState("")
  const [muteSeconds, setMuteSeconds] = useState("0")
  const [applyIdInput, setApplyIdInput] = useState("")
  const [announcement, setAnnouncement] = useState(group.announcement ?? "")
  const [selectedPermissionBits, setSelectedPermissionBits] = useState<number>(GROUP_DEFAULT_ADMIN_PERMISSION_BITS)
  const [localError, setLocalError] = useState("")

  const canChangeInfo = Boolean(group.canChangeInfo || group.role === "owner")
  const canInviteUsers = Boolean(group.canInviteUsers || group.role === "owner")
  const canManageAdmins = Boolean(group.canManageAdmins || group.role === "owner")
  const canBanUsers = Boolean(group.canBanUsers || group.role === "owner")
  const isOwner = group.role === "owner"
  const canReviewApplies = isOwner || group.role === "admin" || canInviteUsers || canManageAdmins
  const hasManageActions = canInviteUsers || canManageAdmins || canBanUsers
  const sortedMembers = useMemo(() => {
    const rank = (role: GroupMemberEntry["role"]) => (role === "owner" ? 0 : role === "admin" ? 1 : 2)
    return [...members].sort((a, b) => {
      const roleDiff = rank(a.role) - rank(b.role)
      if (roleDiff !== 0) return roleDiff
      return a.name.localeCompare(b.name, "zh-CN")
    })
  }, [members])
  const groupPendingApplies = useMemo(
    () => pendingApplies.filter((item) => item.groupId === group.groupId),
    [pendingApplies, group.groupId],
  )
  const permissionOptions = useMemo(() => availablePermissionOptions(group), [group])
  const friendsWithUserId = useMemo(
    () => friends.filter((friend) => Boolean(normalizePublicUserId(friend.userId))),
    [friends],
  )

  useEffect(() => {
    if (!open) return
    setAnnouncement(group.announcement ?? "")
    setLocalError("")
    const defaults = availablePermissionOptions(group)
    let bits = 0
    for (const item of defaults) {
      if (item.defaultChecked) bits |= item.bit
    }
    setSelectedPermissionBits(bits > 0 ? bits : GROUP_DEFAULT_ADMIN_PERMISSION_BITS)
  }, [open, group.groupId, group.announcement, group.canChangeInfo, group.canDeleteMessages, group.canInviteUsers, group.canManageAdmins, group.canPinMessages, group.canBanUsers, group.canManageTopics])

  if (!open) return null

  function requireValidUserId(raw: string): string | null {
    const value = raw.trim()
    if (!USER_ID_PATTERN.test(value)) {
      setLocalError(`用户ID格式非法：${value || "(空)"}`)
      return null
    }
    setLocalError("")
    return value
  }

  function togglePermissionBit(bit: number) {
    setSelectedPermissionBits((current) => (current & bit ? current & ~bit : current | bit))
  }

  async function handleIconPick(file: File | null) {
    if (!file) return
    if (!canChangeInfo) {
      setLocalError("没有修改群资料权限")
      return
    }
    try {
      setLocalError("")
      await actions.updateGroupIconFromFile(file)
    } catch (err) {
      setLocalError(err instanceof Error ? err.message : "群头像上传失败")
    }
  }

  function handleInvite() {
    const userId = requireValidUserId(targetUserId)
    if (!userId) return
    actions.inviteGroupMember(userId, reason)
  }

  function handleKick() {
    const userId = requireValidUserId(targetUserId)
    if (!userId) return
    actions.kickGroupMember(userId)
  }

  function handleSetAdmin(isAdmin: boolean) {
    const userId = requireValidUserId(targetUserId)
    if (!userId) return
    actions.setGroupAdmin(userId, isAdmin, isAdmin ? selectedPermissionBits : 0)
  }

  function handleMute() {
    const userId = requireValidUserId(targetUserId)
    if (!userId) return
    const seconds = Number(muteSeconds)
    if (!Number.isFinite(seconds) || seconds < 0) {
      setLocalError("禁言秒数非法")
      return
    }
    actions.muteGroupMember(userId, seconds)
  }

  function handleSelectMember(member: GroupMemberEntry) {
    const userId = normalizePublicUserId(member.userId) || ""
    if (userId) {
      setTargetUserId(userId)
      setLocalError("")
      return
    }
    setLocalError("该成员缺少公开用户ID，暂无法直接操作")
  }

  function handleReview(applyId: number, agree: boolean) {
    if (!Number.isFinite(applyId) || applyId <= 0) {
      setLocalError("审核单号非法")
      return
    }
    setLocalError("")
    actions.reviewGroupApply(applyId, agree)
  }

  function handleReviewFromInput(agree: boolean) {
    const applyId = Number(applyIdInput.trim())
    if (!Number.isFinite(applyId) || applyId <= 0) {
      setLocalError("审核单号非法")
      return
    }
    handleReview(applyId, agree)
  }

  function memberRoleLabel(role: GroupMemberEntry["role"] | undefined): string {
    if (role === "owner") return "群主"
    if (role === "admin") return "管理员"
    return "成员"
  }

  const displayName = groupDisplayName(group)
  const bannerText = localError || statusText
  const bannerIsError = Boolean(localError) || statusError

  return (
    <div
      role="dialog"
      aria-modal="true"
      aria-label="群管理"
      style={{
        position: "absolute",
        inset: 0,
        zIndex: 20,
        display: "flex",
        justifyContent: "flex-end",
        background: "rgba(18, 24, 34, 0.28)",
        backdropFilter: "blur(2px)",
      }}
      onClick={onClose}
    >
      <GlassSurface
        elevated
        onClick={(event) => event.stopPropagation()}
        style={{
          width: "min(420px, 100%)",
          height: "100%",
          borderRadius: 0,
          display: "flex",
          flexDirection: "column",
          overflow: "hidden",
          boxShadow: "-18px 0 40px rgba(0,0,0,0.14)",
        }}
      >
        <div
          style={{
            padding: "14px 16px",
            borderBottom: "1px solid var(--divider)",
            display: "flex",
            alignItems: "center",
            gap: 10,
            flexShrink: 0,
          }}
        >
          <GlassButton type="button" onClick={onClose} style={{ padding: "5px 10px", fontSize: 12 }}>
            返回
          </GlassButton>
          <div style={{ minWidth: 0, flex: 1 }}>
            <div style={{ fontSize: 16, fontWeight: 700 }}>群管理</div>
            <div
              style={{
                marginTop: 2,
                fontSize: 12,
                color: "var(--text-secondary)",
                overflow: "hidden",
                textOverflow: "ellipsis",
                whiteSpace: "nowrap",
              }}
            >
              {displayName}
            </div>
          </div>
        </div>

        <GlassScrollArea style={{ flex: 1, padding: 14 }}>
          <div style={{ display: "grid", gap: 12 }}>
            <GlassSurface style={{ padding: 14, borderRadius: 12 }}>
              <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
                <Avatar src={group.icon} name={displayName} size={56} />
                <div style={{ minWidth: 0, flex: 1 }}>
                  <div style={{ fontSize: 16, fontWeight: 700, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                    {displayName}
                  </div>
                  <div style={{ marginTop: 4, fontSize: 12, color: "var(--text-secondary)" }}>
                    群号: {groupPublicIdText(group.groupCode)}
                  </div>
                  <div style={{ marginTop: 2, fontSize: 12, color: "var(--text-disabled)" }}>
                    我的身份: {roleLabel(group.role)}
                    {(group.memberCount ?? 0) > 0 ? ` · ${group.memberCount} 人` : ""}
                  </div>
                </div>
              </div>

              <div style={{ marginTop: 12, display: "flex", gap: 8, flexWrap: "wrap" }}>
                <GlassButton
                  type="button"
                  disabled={!canChangeInfo || busy}
                  onClick={() => fileInputRef.current?.click()}
                  style={{ padding: "6px 10px", fontSize: 12 }}
                >
                  修改群头像
                </GlassButton>
                <GlassButton
                  type="button"
                  disabled={busy}
                  onClick={() => actions.refreshGroupList()}
                  style={{ padding: "6px 10px", fontSize: 12 }}
                >
                  刷新
                </GlassButton>
                <input
                  ref={fileInputRef}
                  type="file"
                  accept="image/*"
                  hidden
                  onChange={(event) => {
                    const file = event.target.files?.[0] ?? null
                    event.target.value = ""
                    void handleIconPick(file)
                  }}
                />
              </div>

              {bannerText ? (
                <div
                  style={{
                    marginTop: 10,
                    fontSize: 12,
                    lineHeight: 1.45,
                    color: bannerIsError ? "var(--color-badge)" : "var(--text-secondary)",
                    whiteSpace: "pre-wrap",
                  }}
                >
                  {bannerText}
                </div>
              ) : null}

              <div style={{ marginTop: 12, display: "grid", gap: 8 }}>
                <GlassTextField
                  value={announcement}
                  onChange={(event) => setAnnouncement(event.target.value)}
                  placeholder="群公告（最多1000字）"
                  aria-label="群公告"
                  disabled={!canChangeInfo || busy}
                />
                <div style={{ display: "flex", gap: 8 }}>
                  <GlassButton
                    type="button"
                    variant="primary"
                    disabled={!canChangeInfo || busy}
                    onClick={() => actions.updateGroupAnnouncement(announcement)}
                    style={{ flex: 1 }}
                  >
                    更新公告
                  </GlassButton>
                  {!isOwner ? (
                    <GlassButton
                      type="button"
                      variant="danger"
                      disabled={busy}
                      onClick={() => actions.quitGroup()}
                      style={{ flex: 1 }}
                    >
                      退群
                    </GlassButton>
                  ) : null}
                </div>
                {isOwner ? (
                  <GlassButton
                    type="button"
                    variant="danger"
                    disabled={busy}
                    onClick={() => {
                      if (window.confirm(`确认解散群聊「${displayName}」？此操作不可撤销。`)) {
                        actions.dissolveGroup()
                      }
                    }}
                  >
                    解散群聊
                  </GlassButton>
                ) : (
                  <div style={{ fontSize: 11, color: "var(--text-disabled)" }}>仅群主可解散群聊</div>
                )}
              </div>
            </GlassSurface>


            <GlassSurface style={{ padding: 14, borderRadius: 12 }}>
              <div style={{ display: "flex", alignItems: "baseline", justifyContent: "space-between", gap: 8, marginBottom: 10 }}>
                <div style={{ fontSize: 14, fontWeight: 700 }}>群成员</div>
                <div style={{ fontSize: 12, color: "var(--text-disabled)" }}>
                  {sortedMembers.length > 0
                    ? `已识别 ${sortedMembers.length} 人`
                    : (group.memberCount ?? 0) > 0
                      ? `共 ${group.memberCount} 人（聊天记录补齐中）`
                      : "暂无成员数据"}
                </div>
              </div>
              {sortedMembers.length === 0 ? (
                <div style={{ fontSize: 12, color: "var(--text-disabled)", lineHeight: 1.5 }}>
                  当前协议未下发完整成员表。已展示本地可识别成员（自己/好友/群消息发送者），打开群聊或刷新后会继续补齐。
                </div>
              ) : (
                <div style={{ display: "grid", gap: 8 }}>
                  {sortedMembers.map((member) => {
                    const userId = publicUserIdText(member.userId)
                    const selected = Boolean(userId) && targetUserId.trim() === userId
                    return (
                      <button
                        key={`${member.uid}-${userId || member.name}`}
                        type="button"
                        onClick={() => handleSelectMember(member)}
                        disabled={busy}
                        style={{
                          display: "flex",
                          alignItems: "center",
                          gap: 10,
                          width: "100%",
                          textAlign: "left",
                          borderRadius: 10,
                          border: selected ? "1px solid var(--color-accent, #6aa8ff)" : "1px solid var(--divider)",
                          background: selected ? "var(--tint-selected)" : "rgba(255,255,255,0.08)",
                          padding: "8px 10px",
                          cursor: busy ? "not-allowed" : "pointer",
                          color: "inherit",
                          font: "inherit",
                        }}
                      >
                        <Avatar src={member.icon} name={member.name} size={34} />
                        <div style={{ minWidth: 0, flex: 1 }}>
                          <div style={{ fontSize: 13, fontWeight: 600, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                            {member.name}
                          </div>
                          <div style={{ marginTop: 2, fontSize: 11, color: "var(--text-disabled)" }}>
                            {memberRoleLabel(member.role)}
                            {userId ? ` · ${userId}` : ""}
                          </div>
                        </div>
                      </button>
                    )
                  })}
                </div>
              )}
            </GlassSurface>

            {canReviewApplies ? (
              <GlassSurface style={{ padding: 14, borderRadius: 12 }}>
                <div style={{ fontSize: 14, fontWeight: 700, marginBottom: 10 }}>入群审核</div>
                {groupPendingApplies.length === 0 ? (
                  <div style={{ fontSize: 12, color: "var(--text-disabled)", marginBottom: 10 }}>
                    暂无待审申请。也可手动填写 ApplyId 审核。
                  </div>
                ) : (
                  <div style={{ display: "grid", gap: 8, marginBottom: 10 }}>
                    {groupPendingApplies.map((apply) => {
                      const applicantLabel = apply.applicantUserId
                        || (apply.applicantUid > 0 ? `uid:${apply.applicantUid}` : "未知申请人")
                      return (
                        <div
                          key={apply.applyId}
                          style={{
                            border: "1px solid var(--divider)",
                            borderRadius: 10,
                            padding: "10px 10px",
                            background: "rgba(255,255,255,0.08)",
                            display: "grid",
                            gap: 8,
                          }}
                        >
                          <div style={{ fontSize: 13, fontWeight: 600 }}>
                            申请单 #{apply.applyId}
                          </div>
                          <div style={{ fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.45 }}>
                            申请人: {applicantLabel}
                            {apply.reason ? ` · ${apply.reason}` : ""}
                          </div>
                          <div style={{ display: "flex", gap: 8 }}>
                            <GlassButton
                              type="button"
                              disabled={busy}
                              onClick={() => handleReview(apply.applyId, true)}
                              style={{ flex: 1 }}
                            >
                              同意
                            </GlassButton>
                            <GlassButton
                              type="button"
                              variant="danger"
                              disabled={busy}
                              onClick={() => handleReview(apply.applyId, false)}
                              style={{ flex: 1 }}
                            >
                              拒绝
                            </GlassButton>
                          </div>
                        </div>
                      )
                    })}
                  </div>
                )}
                <div style={{ display: "grid", gap: 8 }}>
                  <GlassTextField
                    value={applyIdInput}
                    onChange={(event) => setApplyIdInput(event.target.value)}
                    placeholder="审核单 ApplyId"
                    aria-label="审核单 ApplyId"
                    disabled={busy}
                  />
                  <div style={{ display: "flex", gap: 8 }}>
                    <GlassButton type="button" disabled={busy} onClick={() => handleReviewFromInput(true)} style={{ flex: 1 }}>
                      同意
                    </GlassButton>
                    <GlassButton type="button" variant="danger" disabled={busy} onClick={() => handleReviewFromInput(false)} style={{ flex: 1 }}>
                      拒绝
                    </GlassButton>
                  </div>
                </div>
              </GlassSurface>
            ) : null}

            <GlassSurface style={{ padding: 14, borderRadius: 12 }}>
              <div style={{ fontSize: 14, fontWeight: 700, marginBottom: 10 }}>成员管理</div>
              {!hasManageActions ? (
                <div style={{ fontSize: 12, color: "var(--text-disabled)" }}>暂无可用管理权限</div>
              ) : (
                <div style={{ display: "grid", gap: 8 }}>
                  <label style={{ fontSize: 12, color: "var(--text-secondary)" }}>
                    选择好友
                    <select
                      value={targetUserId}
                      onChange={(event) => setTargetUserId(event.target.value)}
                      disabled={busy}
                      style={{
                        display: "block",
                        width: "100%",
                        marginTop: 4,
                        borderRadius: 10,
                        border: "1px solid var(--divider)",
                        background: "rgba(255,255,255,0.10)",
                        color: "var(--text-primary)",
                        padding: "8px 10px",
                        font: "inherit",
                        fontSize: 13,
                      }}
                    >
                      <option value="">选择好友</option>
                      {friendsWithUserId.map((friend) => {
                        const userId = publicUserIdText(friend.userId)
                        return (
                          <option key={friend.uid} value={userId}>
                            {friendDisplayName(friend)} ({userId})
                          </option>
                        )
                      })}
                    </select>
                  </label>

                  <GlassTextField
                    value={targetUserId}
                    onChange={(event) => setTargetUserId(event.target.value)}
                    placeholder="目标用户ID（可手动补充）"
                    aria-label="目标用户ID"
                    disabled={busy}
                  />

                  {canInviteUsers ? (
                    <GlassTextField
                      value={reason}
                      onChange={(event) => setReason(event.target.value)}
                      placeholder="理由（可选）"
                      aria-label="邀请理由"
                      disabled={busy}
                    />
                  ) : null}

                  {canBanUsers ? (
                    <GlassTextField
                      value={muteSeconds}
                      onChange={(event) => setMuteSeconds(event.target.value)}
                      placeholder="禁言秒数（0=解禁）"
                      aria-label="禁言秒数"
                      disabled={busy}
                    />
                  ) : null}

                  {canManageAdmins ? (
                    <div style={{ display: "flex", flexWrap: "wrap", gap: 6 }}>
                      {permissionOptions.map((item) => {
                        const checked = (selectedPermissionBits & item.bit) !== 0
                        return (
                          <label
                            key={item.bit}
                            style={{
                              display: "inline-flex",
                              alignItems: "center",
                              gap: 6,
                              padding: "5px 8px",
                              borderRadius: 8,
                              border: "1px solid var(--divider)",
                              background: checked ? "var(--tint-selected)" : "rgba(255,255,255,0.08)",
                              fontSize: 12,
                              cursor: busy ? "not-allowed" : "pointer",
                            }}
                          >
                            <input
                              type="checkbox"
                              checked={checked}
                              disabled={busy}
                              onChange={() => togglePermissionBit(item.bit)}
                            />
                            {item.label}
                          </label>
                        )
                      })}
                    </div>
                  ) : null}

                  <div style={{ display: "flex", gap: 8, flexWrap: "wrap" }}>
                    {canInviteUsers ? (
                      <GlassButton type="button" disabled={busy} onClick={handleInvite} style={{ flex: 1, minWidth: 96 }}>
                        邀请
                      </GlassButton>
                    ) : null}
                    {canManageAdmins ? (
                      <GlassButton type="button" disabled={busy} onClick={() => handleSetAdmin(true)} style={{ flex: 1, minWidth: 96 }}>
                        设管理员
                      </GlassButton>
                    ) : null}
                    {canManageAdmins ? (
                      <GlassButton type="button" disabled={busy} onClick={() => handleSetAdmin(false)} style={{ flex: 1, minWidth: 96 }}>
                        撤管理员
                      </GlassButton>
                    ) : null}
                    {canBanUsers ? (
                      <GlassButton type="button" disabled={busy} onClick={handleMute} style={{ flex: 1, minWidth: 96 }}>
                        禁言
                      </GlassButton>
                    ) : null}
                    {canBanUsers ? (
                      <GlassButton type="button" variant="danger" disabled={busy} onClick={handleKick} style={{ flex: 1, minWidth: 96 }}>
                        踢出群聊
                      </GlassButton>
                    ) : null}
                  </div>
                </div>
              )}
            </GlassSurface>
          </div>
        </GlassScrollArea>
      </GlassSurface>
    </div>
  )
}
