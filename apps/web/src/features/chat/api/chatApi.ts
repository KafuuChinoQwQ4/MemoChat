/** Chat API — send/edit/revoke/forward private + group messages, dialog list */
import type { HttpClient } from "@/core/network/http/HttpClient"
import type { ChatTransport } from "@/core/network/transport/ChatTransport"
import { ReqId } from "@/core/network/opcodes/reqIds"
import { ENDPOINTS } from "@/core/config/endpoints"
import { genClientMsgId } from "@/shared/lib/id"
import { normalizeAdminPermissionBits } from "@/app/dispatch/chatListPayloads"

export type SmartFeatureType = "summary" | "suggest" | "translate"

export interface SmartFeatureResponse {
  code?: number
  error?: number
  message?: string
  result?: string
  content?: string
}

export function createChatApi(transport: ChatTransport, _http: HttpClient) {
  return {
    /** Search a user by public user id, e.g. u123456789 */
    searchUser(userId: string) {
      transport.send(ReqId.ID_SEARCH_USER_REQ, JSON.stringify({
        user_id: userId.trim(),
      }))
    },

    /** Send a friend apply request */
    sendAddFriendApply(fromUid: number, applyName: string, targetUid: number, remark = "", labels: string[] = []) {
      const cleanName = applyName.trim() || "未知用户"
      transport.send(ReqId.ID_ADD_FRIEND_REQ, JSON.stringify({
        uid: fromUid,
        applyname: cleanName,
        bakname: remark.trim() || cleanName,
        touid: targetUid,
        labels: labels.map((item) => item.trim()).filter(Boolean),
      }))
    },

    /** Approve an incoming friend apply */
    approveFriendApply(fromUid: number, targetUid: number, remark = "", labels: string[] = []) {
      transport.send(ReqId.ID_AUTH_FRIEND_REQ, JSON.stringify({
        fromuid: fromUid,
        touid: targetUid,
        back: remark.trim(),
        labels: labels.map((item) => item.trim()).filter(Boolean),
      }))
    },

    /** Create a group using public user ids, matching the desktop client payload */
    createGroup(fromUid: number, name: string, memberUserIds: string[], memberLimit = 200) {
      transport.send(ReqId.ID_CREATE_GROUP_REQ, JSON.stringify({
        fromuid: fromUid,
        name: name.trim(),
        member_limit: memberLimit,
        member_user_ids: memberUserIds.map((item) => item.trim()).filter(Boolean),
      }))
    },

    /** Fetch group list */
    fetchGroupList(fromUid: number) {
      transport.send(ReqId.ID_GET_GROUP_LIST_REQ, JSON.stringify({ fromuid: fromUid }))
    },

    /** Invite a user into a group by public user id */
    inviteGroupMember(fromUid: number, groupId: number, targetUserId: string, reason = "") {
      transport.send(ReqId.ID_INVITE_GROUP_MEMBER_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        target_user_id: targetUserId.trim(),
        reason: reason.trim(),
      }))
    },

    /** Review a pending group join apply (desktop ID_REVIEW_GROUP_APPLY_REQ) */
    reviewGroupApply(fromUid: number, applyId: number, agree: boolean) {
      transport.send(ReqId.ID_REVIEW_GROUP_APPLY_REQ, JSON.stringify({
        fromuid: fromUid,
        apply_id: applyId,
        agree,
      }))
    },

    /** Kick a group member by public user id */
    kickGroupMember(fromUid: number, groupId: number, targetUserId: string) {
      transport.send(ReqId.ID_KICK_GROUP_MEMBER_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        target_user_id: targetUserId.trim(),
      }))
    },

    /** Quit a group (non-owner) */
    quitGroup(fromUid: number, groupId: number) {
      transport.send(ReqId.ID_QUIT_GROUP_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
      }))
    },

    /** Dissolve a group (owner only) */
    dissolveGroup(fromUid: number, groupId: number) {
      transport.send(ReqId.ID_DISSOLVE_GROUP_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
      }))
    },

    /** Update group announcement */
    updateGroupAnnouncement(fromUid: number, groupId: number, announcement: string) {
      transport.send(ReqId.ID_UPDATE_GROUP_ANNOUNCEMENT_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        announcement: announcement.slice(0, 1000),
      }))
    },

    /** Update group icon URL after media upload */
    updateGroupIcon(fromUid: number, groupId: number, icon: string) {
      transport.send(ReqId.ID_UPDATE_GROUP_ICON_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        icon,
      }))
    },

    /** Promote/demote a group admin with desktop permission bits */
    setGroupAdmin(
      fromUid: number,
      groupId: number,
      targetUserId: string,
      isAdmin: boolean,
      permissionBits = 0,
    ) {
      const bits = normalizeAdminPermissionBits(isAdmin, permissionBits)
      transport.send(ReqId.ID_SET_GROUP_ADMIN_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        target_user_id: targetUserId.trim(),
        is_admin: isAdmin,
        permission_bits: bits,
        can_change_group_info: isAdmin && (bits & 1) !== 0,
        can_delete_messages: isAdmin && (bits & 2) !== 0,
        can_invite_users: isAdmin && (bits & 4) !== 0,
        can_manage_admins: isAdmin && (bits & 8) !== 0,
        can_pin_messages: isAdmin && (bits & 16) !== 0,
        can_ban_users: isAdmin && (bits & 32) !== 0,
        can_manage_topics: isAdmin && (bits & 64) !== 0,
      }))
    },

    /** Mute/unmute a group member (muteSeconds=0 means unmute) */
    muteGroupMember(fromUid: number, groupId: number, targetUserId: string, muteSeconds: number) {
      transport.send(ReqId.ID_MUTE_GROUP_MEMBER_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        target_user_id: targetUserId.trim(),
        mute_seconds: Math.max(0, Math.floor(muteSeconds) || 0),
      }))
    },

    /** Run desktop-equivalent smart chat features through the AI gateway */
    async runSmartFeature(
      uid: number,
      featureType: SmartFeatureType,
      content: string,
      options: {
        targetLang?: string
        context?: Record<string, unknown>
        modelType?: string
        modelName?: string
      } = {},
    ): Promise<SmartFeatureResponse> {
      const payload: Record<string, unknown> = {
        uid,
        feature_type: featureType,
        content,
        model_type: options.modelType ?? "",
        model_name: options.modelName ?? "",
        deployment_preference: "any",
        context_json: JSON.stringify(options.context ?? {}),
      }
      if (featureType === "translate") {
        payload["target_lang"] = options.targetLang?.trim() || "中文"
      }
      return _http.post<SmartFeatureResponse>(ENDPOINTS.aiSmart, payload)
    },

    /** Send a private text message */
    sendPrivateMessage(fromUid: number, toUid: number, content: string): string {
      const clientMsgId = genClientMsgId()
      const createdAt = Date.now()
      transport.send(ReqId.ID_TEXT_CHAT_MSG_REQ, JSON.stringify({
        fromuid: fromUid,
        touid: toUid,
        text_array: [{
          msgid: clientMsgId,
          content,
          created_at: createdAt,
        }],
      }))
      return clientMsgId
    },

    /** Send a group text message */
    sendGroupMessage(fromUid: number, groupId: number, content: string): string {
      const clientMsgId = genClientMsgId()
      transport.send(ReqId.ID_GROUP_CHAT_MSG_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        msg: {
          msgid: clientMsgId,
          content,
          msgtype: "text",
        },
      }))
      return clientMsgId
    },

    /** Fetch private chat history */
    fetchPrivateHistory(fromUid: number, peerUid: number, beforeTs = 0, beforeMsgId = "", limit = 50) {
      const payload: Record<string, number | string> = {
        fromuid: fromUid,
        peer_uid: peerUid,
        before_ts: beforeTs,
        limit,
      }
      if (beforeMsgId.trim()) payload["before_msg_id"] = beforeMsgId.trim()
      transport.send(ReqId.ID_PRIVATE_HISTORY_REQ, JSON.stringify(payload))
    },

    /** Fetch group chat history */
    fetchGroupHistory(fromUid: number, groupId: number, beforeSeq = 0, limit = 50) {
      transport.send(ReqId.ID_GROUP_HISTORY_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        before_ts: 0,
        before_seq: beforeSeq,
        limit,
      }))
    },

    /** Fetch dialog list */
    fetchDialogList(page = 1) {
      transport.send(ReqId.ID_GET_DIALOG_LIST_REQ, JSON.stringify({ page }))
    },

    /** Send read ack for private conversation */
    sendReadAck(peerUid: number, msgId: number) {
      transport.send(ReqId.ID_PRIVATE_READ_ACK_REQ, JSON.stringify({
        peer_uid: peerUid,
        last_read_msg_id: msgId,
      }))
    },

    /** Revoke a private message */
    revokePrivateMessage(peerUid: number, msgId: number) {
      transport.send(ReqId.ID_REVOKE_PRIVATE_MSG_REQ, JSON.stringify({ peer_uid: peerUid, msg_id: msgId }))
    },
  }
}

export type ChatApi = ReturnType<typeof createChatApi>
