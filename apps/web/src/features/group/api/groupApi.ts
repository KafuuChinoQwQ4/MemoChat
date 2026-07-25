/** Group API — desktop-aligned group management request payloads. */
import type { ChatTransport } from "@/core/network/transport/ChatTransport"
import { ReqId } from "@/core/network/opcodes/reqIds"
import { normalizeAdminPermissionBits } from "@/core/network/opcodes/protocol"

export function createGroupApi(transport: ChatTransport) {
  return {
    fetchGroupList(fromUid: number) {
      transport.send(ReqId.ID_GET_GROUP_LIST_REQ, JSON.stringify({ fromuid: fromUid }))
    },

    inviteGroupMember(fromUid: number, groupId: number, targetUserId: string, reason = "") {
      transport.send(ReqId.ID_INVITE_GROUP_MEMBER_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        target_user_id: targetUserId.trim(),
        reason: reason.trim(),
      }))
    },

    reviewGroupApply(fromUid: number, applyId: number, agree: boolean) {
      transport.send(ReqId.ID_REVIEW_GROUP_APPLY_REQ, JSON.stringify({
        fromuid: fromUid,
        apply_id: applyId,
        agree,
      }))
    },

    kickGroupMember(fromUid: number, groupId: number, targetUserId: string) {
      transport.send(ReqId.ID_KICK_GROUP_MEMBER_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        target_user_id: targetUserId.trim(),
      }))
    },

    quitGroup(fromUid: number, groupId: number) {
      transport.send(ReqId.ID_QUIT_GROUP_REQ, JSON.stringify({ fromuid: fromUid, groupid: groupId }))
    },

    dissolveGroup(fromUid: number, groupId: number) {
      transport.send(ReqId.ID_DISSOLVE_GROUP_REQ, JSON.stringify({ fromuid: fromUid, groupid: groupId }))
    },

    updateGroupAnnouncement(fromUid: number, groupId: number, announcement: string) {
      transport.send(ReqId.ID_UPDATE_GROUP_ANNOUNCEMENT_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        announcement: announcement.slice(0, 1000),
      }))
    },

    updateGroupIcon(fromUid: number, groupId: number, icon: string) {
      transport.send(ReqId.ID_UPDATE_GROUP_ICON_REQ, JSON.stringify({ fromuid: fromUid, groupid: groupId, icon }))
    },

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

    muteGroupMember(fromUid: number, groupId: number, targetUserId: string, muteSeconds: number) {
      transport.send(ReqId.ID_MUTE_GROUP_MEMBER_REQ, JSON.stringify({
        fromuid: fromUid,
        groupid: groupId,
        target_user_id: targetUserId.trim(),
        mute_seconds: Math.max(0, Math.floor(muteSeconds) || 0),
      }))
    },
  }
}
