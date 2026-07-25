import { describe, expect, it } from "vitest"
import { ReqId } from "@/core/network/opcodes/reqIds"
import type { ChatTransport } from "@/core/network/transport/ChatTransport"
import { createGroupApi } from "./groupApi"

describe("groupApi", () => {
  it("sends group management payloads in the desktop wire format", () => {
    const sent: Array<{ reqId: ReqId; payload: unknown }> = []
    const transport: ChatTransport = {
      connect: () => undefined,
      close: () => undefined,
      isConnected: () => true,
      send: (reqId, payload) => sent.push({ reqId, payload: JSON.parse(payload) as unknown }),
    }

    const api = createGroupApi(transport)
    api.fetchGroupList(910001)
    api.inviteGroupMember(910001, 83, " u323456789 ", " join us ")
    api.reviewGroupApply(910001, 42, true)
    api.kickGroupMember(910001, 83, "u323456789")
    api.quitGroup(910001, 83)
    api.dissolveGroup(910001, 83)
    api.updateGroupAnnouncement(910001, 83, "hello")
    api.updateGroupIcon(910001, 83, "https://cdn.example/group.png")
    api.setGroupAdmin(910001, 83, "u323456789", true, 0)
    api.muteGroupMember(910001, 83, "u323456789", 60)

    expect(sent).toEqual([
      { reqId: ReqId.ID_GET_GROUP_LIST_REQ, payload: { fromuid: 910001 } },
      {
        reqId: ReqId.ID_INVITE_GROUP_MEMBER_REQ,
        payload: { fromuid: 910001, groupid: 83, target_user_id: "u323456789", reason: "join us" },
      },
      {
        reqId: ReqId.ID_REVIEW_GROUP_APPLY_REQ,
        payload: { fromuid: 910001, apply_id: 42, agree: true },
      },
      {
        reqId: ReqId.ID_KICK_GROUP_MEMBER_REQ,
        payload: { fromuid: 910001, groupid: 83, target_user_id: "u323456789" },
      },
      { reqId: ReqId.ID_QUIT_GROUP_REQ, payload: { fromuid: 910001, groupid: 83 } },
      { reqId: ReqId.ID_DISSOLVE_GROUP_REQ, payload: { fromuid: 910001, groupid: 83 } },
      {
        reqId: ReqId.ID_UPDATE_GROUP_ANNOUNCEMENT_REQ,
        payload: { fromuid: 910001, groupid: 83, announcement: "hello" },
      },
      {
        reqId: ReqId.ID_UPDATE_GROUP_ICON_REQ,
        payload: { fromuid: 910001, groupid: 83, icon: "https://cdn.example/group.png" },
      },
      {
        reqId: ReqId.ID_SET_GROUP_ADMIN_REQ,
        payload: {
          fromuid: 910001,
          groupid: 83,
          target_user_id: "u323456789",
          is_admin: true,
          permission_bits: 55,
          can_change_group_info: true,
          can_delete_messages: true,
          can_invite_users: true,
          can_manage_admins: false,
          can_pin_messages: true,
          can_ban_users: true,
          can_manage_topics: false,
        },
      },
      {
        reqId: ReqId.ID_MUTE_GROUP_MEMBER_REQ,
        payload: { fromuid: 910001, groupid: 83, target_user_id: "u323456789", mute_seconds: 60 },
      },
    ])
  })
})
