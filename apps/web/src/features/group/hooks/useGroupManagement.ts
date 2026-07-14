/** useGroupManagement — wire desktop-aligned group management ops + response status */
import { useCallback, useEffect, useMemo, useState } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { useSessionStore } from "@/core/session/sessionStore"
import { ReqId } from "@/core/network/opcodes/reqIds"
import { createChatApi } from "@/features/chat/api/chatApi"
import { useChatStore } from "@/features/chat/store/chatStore"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { uploadMedia } from "@/shared/media/MediaUploadService"
import type { GroupManagementActions } from "@/features/group/components/GroupManagementPanel"

interface GroupOpStatus {
  text: string
  isError: boolean
}

function actionText(reqId: ReqId): string {
  switch (reqId) {
    case ReqId.ID_INVITE_GROUP_MEMBER_RSP: return "邀请成员"
    case ReqId.ID_APPLY_JOIN_GROUP_RSP: return "申请入群"
    case ReqId.ID_REVIEW_GROUP_APPLY_RSP: return "审核申请"
    case ReqId.ID_UPDATE_GROUP_ANNOUNCEMENT_RSP: return "更新群公告"
    case ReqId.ID_UPDATE_GROUP_ICON_RSP: return "更新群头像"
    case ReqId.ID_MUTE_GROUP_MEMBER_RSP: return "禁言成员"
    case ReqId.ID_SET_GROUP_ADMIN_RSP: return "设置管理员"
    case ReqId.ID_KICK_GROUP_MEMBER_RSP: return "踢出成员"
    case ReqId.ID_QUIT_GROUP_RSP: return "退出群聊"
    case ReqId.ID_DISSOLVE_GROUP_RSP: return "解散群聊"
    default: return "群操作"
  }
}

function payloadGroupId(data: Record<string, unknown>): number {
  const raw = data["groupid"] ?? data["group_id"]
  if (typeof raw === "number" && Number.isFinite(raw)) return raw
  if (typeof raw === "string" && raw.trim()) {
    const n = Number(raw)
    if (Number.isFinite(n)) return n
  }
  return 0
}

function removeGroupLocally(groupId: number): void {
  if (groupId <= 0) return
  const store = useEntityStore.getState()
  store.removeGroup(groupId)
  store.removeDialog(groupId)
  const chat = useChatStore.getState()
  if (chat.selectedPeerId === groupId && chat.selectedIsGroup) {
    chat.reset()
  }
}

export function useGroupManagement(groupId: number | null) {
  const uid = useSessionStore((s) => s.uid) ?? 0
  const gateway = useMemo(() => getGateway(), [])
  const api = useMemo(() => createChatApi(gateway.chatTransport, gateway.http), [gateway])
  const [status, setStatus] = useState<GroupOpStatus>({ text: "", isError: false })
  const [busy, setBusy] = useState(false)

  useEffect(() => {
    setStatus({ text: "", isError: false })
    setBusy(false)
  }, [groupId])

  useEffect(() => {
    const managementRsp = [
      ReqId.ID_INVITE_GROUP_MEMBER_RSP,
      ReqId.ID_REVIEW_GROUP_APPLY_RSP,
      ReqId.ID_UPDATE_GROUP_ANNOUNCEMENT_RSP,
      ReqId.ID_UPDATE_GROUP_ICON_RSP,
      ReqId.ID_MUTE_GROUP_MEMBER_RSP,
      ReqId.ID_SET_GROUP_ADMIN_RSP,
      ReqId.ID_KICK_GROUP_MEMBER_RSP,
      ReqId.ID_QUIT_GROUP_RSP,
      ReqId.ID_DISSOLVE_GROUP_RSP,
    ] as const

    const unsubs = managementRsp.map((reqId) =>
      gateway.dispatcher.subscribe(reqId, (frame) => {
        try {
          const data = JSON.parse(frame.payload) as Record<string, unknown>
          const error = typeof data.error === "number" ? data.error : 0
          const action = actionText(reqId)
          const responseGroupId = payloadGroupId(data)
          if (groupId && responseGroupId > 0 && responseGroupId !== groupId) {
            // Ignore responses for other groups while this panel is open.
            if (reqId !== ReqId.ID_QUIT_GROUP_RSP && reqId !== ReqId.ID_DISSOLVE_GROUP_RSP) {
              return
            }
          }

          if (error !== 0) {
            const serverMessage = typeof data.message === "string" ? data.message.trim() : ""
            setStatus({
              text: serverMessage
                ? `${action}失败：${serverMessage}（错误码:${error}）`
                : `${action}失败（错误码:${error}）`,
              isError: true,
            })
            setBusy(false)
            return
          }

          switch (reqId) {
            case ReqId.ID_INVITE_GROUP_MEMBER_RSP:
              setStatus({ text: "邀请成员成功", isError: false })
              if (uid > 0) api.fetchGroupList(uid)
              break
            case ReqId.ID_REVIEW_GROUP_APPLY_RSP: {
              const applyId = typeof data.apply_id === "number"
                ? data.apply_id
                : typeof data.apply_id === "string"
                  ? Number(data.apply_id)
                  : 0
              if (Number.isFinite(applyId) && applyId > 0) {
                useEntityStore.getState().removePendingGroupApply(applyId)
              }
              const agree = data.agree === true || data.agree === 1 || data.agree === "true"
              setStatus({ text: agree ? "已同意入群申请" : "已拒绝入群申请", isError: false })
              if (uid > 0) api.fetchGroupList(uid)
              break
            }
            case ReqId.ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
              setStatus({ text: "群公告已更新", isError: false })
              if (uid > 0) api.fetchGroupList(uid)
              break
            case ReqId.ID_UPDATE_GROUP_ICON_RSP: {
              const icon = typeof data.icon === "string" ? data.icon : ""
              const gid = responseGroupId || groupId || 0
              if (gid > 0 && icon) {
                const existing = useEntityStore.getState().groups.get(gid)
                if (existing) {
                  useEntityStore.getState().upsertGroup({ ...existing, icon })
                }
                const dialog = useEntityStore.getState().dialogs.get(gid)
                if (dialog?.isGroup) {
                  useEntityStore.getState().upsertDialog({ ...dialog, avatar: icon })
                }
              }
              setStatus({ text: "群头像已更新", isError: false })
              if (uid > 0) api.fetchGroupList(uid)
              break
            }
            case ReqId.ID_MUTE_GROUP_MEMBER_RSP:
              setStatus({ text: "禁言设置已生效", isError: false })
              break
            case ReqId.ID_SET_GROUP_ADMIN_RSP:
              setStatus({ text: "管理员设置已更新", isError: false })
              if (uid > 0) api.fetchGroupList(uid)
              break
            case ReqId.ID_KICK_GROUP_MEMBER_RSP:
              setStatus({ text: "成员已移出群聊", isError: false })
              if (uid > 0) api.fetchGroupList(uid)
              break
            case ReqId.ID_QUIT_GROUP_RSP: {
              const gid = responseGroupId || groupId || 0
              setStatus({ text: "已退出当前群聊", isError: false })
              removeGroupLocally(gid)
              if (uid > 0) api.fetchGroupList(uid)
              break
            }
            case ReqId.ID_DISSOLVE_GROUP_RSP: {
              const gid = responseGroupId || groupId || 0
              setStatus({ text: "群聊已解散", isError: false })
              removeGroupLocally(gid)
              if (uid > 0) api.fetchGroupList(uid)
              break
            }
            default:
              break
          }
          setBusy(false)
        } catch {
          setStatus({ text: "群操作响应解析失败", isError: true })
          setBusy(false)
        }
      }),
    )

    return () => {
      for (const unsub of unsubs) unsub()
    }
  }, [api, gateway.dispatcher, groupId, uid])

  const requireReady = useCallback((): boolean => {
    if (uid <= 0) {
      setStatus({ text: "登录状态无效，请重新登录", isError: true })
      return false
    }
    if (!groupId || groupId <= 0) {
      setStatus({ text: "未选中群聊", isError: true })
      return false
    }
    return true
  }, [groupId, uid])

  const actions: GroupManagementActions = useMemo(() => ({
    inviteGroupMember(targetUserId, reason = "") {
      if (!requireReady() || !groupId) return
      setBusy(true)
      setStatus({ text: "正在邀请成员...", isError: false })
      api.inviteGroupMember(uid, groupId, targetUserId, reason)
    },
    reviewGroupApply(applyId, agree) {
      if (uid <= 0) {
        setStatus({ text: "登录状态无效，请重新登录", isError: true })
        return
      }
      const id = Number(applyId)
      if (!Number.isFinite(id) || id <= 0) {
        setStatus({ text: "审核单号非法", isError: true })
        return
      }
      setBusy(true)
      setStatus({ text: agree ? "正在同意入群申请..." : "正在拒绝入群申请...", isError: false })
      api.reviewGroupApply(uid, id, agree)
    },
    kickGroupMember(targetUserId) {
      if (!requireReady() || !groupId) return
      setBusy(true)
      setStatus({ text: "正在踢出成员...", isError: false })
      api.kickGroupMember(uid, groupId, targetUserId)
    },
    setGroupAdmin(targetUserId, isAdmin, permissionBits = 0) {
      if (!requireReady() || !groupId) return
      setBusy(true)
      setStatus({ text: isAdmin ? "正在设置管理员..." : "正在撤销管理员...", isError: false })
      api.setGroupAdmin(uid, groupId, targetUserId, isAdmin, permissionBits)
    },
    muteGroupMember(targetUserId, muteSeconds) {
      if (!requireReady() || !groupId) return
      setBusy(true)
      setStatus({ text: muteSeconds > 0 ? "正在禁言成员..." : "正在解除禁言...", isError: false })
      api.muteGroupMember(uid, groupId, targetUserId, muteSeconds)
    },
    updateGroupAnnouncement(announcement) {
      if (!requireReady() || !groupId) return
      setBusy(true)
      setStatus({ text: "正在更新群公告...", isError: false })
      api.updateGroupAnnouncement(uid, groupId, announcement)
    },
    async updateGroupIconFromFile(file) {
      if (!requireReady() || !groupId) {
        throw new Error("未选中群聊")
      }
      setBusy(true)
      setStatus({ text: "群头像上传中...", isError: false })
      try {
        const uploaded = await uploadMedia({ file })
        const icon = (uploaded.url || uploaded.key || "").trim()
        if (!icon) {
          throw new Error("群头像上传失败：空地址")
        }
        setStatus({ text: "群头像更新中...", isError: false })
        api.updateGroupIcon(uid, groupId, icon)
      } catch (err) {
        setBusy(false)
        const message = err instanceof Error ? err.message : "群头像上传失败"
        setStatus({ text: message, isError: true })
        throw err instanceof Error ? err : new Error(message)
      }
    },
    quitGroup() {
      if (!requireReady() || !groupId) return
      setBusy(true)
      setStatus({ text: "正在退出群聊...", isError: false })
      api.quitGroup(uid, groupId)
    },
    dissolveGroup() {
      if (!requireReady() || !groupId) return
      setBusy(true)
      setStatus({ text: "正在解散群聊...", isError: false })
      api.dissolveGroup(uid, groupId)
    },
    refreshGroupList() {
      if (uid <= 0) {
        setStatus({ text: "登录状态无效，请重新登录", isError: true })
        return
      }
      setStatus({ text: "正在刷新群列表...", isError: false })
      api.fetchGroupList(uid)
    },
  }), [api, groupId, requireReady, uid])

  return {
    actions,
    statusText: status.text,
    statusError: status.isError,
    busy,
    clearStatus: () => setStatus({ text: "", isError: false }),
  }
}
