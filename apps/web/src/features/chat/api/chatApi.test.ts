import { describe, expect, it, vi } from "vitest"
import { ReqId } from "@/core/network/opcodes/reqIds"
import type { ChatTransport } from "@/core/network/transport/ChatTransport"
import type { HttpClient } from "@/core/network/http/HttpClient"
import { createChatApi } from "./chatApi"

describe("chatApi history requests", () => {
  it("sends relation and group management payloads in the desktop wire format", () => {
    const sent: Array<{ reqId: ReqId; payload: unknown }> = []
    const transport: ChatTransport = {
      connect: () => undefined,
      close: () => undefined,
      isConnected: () => true,
      send: (reqId, payload) => sent.push({ reqId, payload: JSON.parse(payload) as unknown }),
    }

    const api = createChatApi(transport, {} as HttpClient)
    api.searchUser(" u123456789 ")
    api.sendAddFriendApply(910001, "Alice", 910002, "memo", ["classmate", ""])
    api.approveFriendApply(910001, 910003, "Bob", ["team"])
    api.createGroup(910001, " Runtime Team ", ["u123456789", " u223456789 "])
    api.inviteGroupMember(910001, 83, " u323456789 ", " join us ")
    api.kickGroupMember(910001, 83, "u323456789")
    api.quitGroup(910001, 83)
    api.dissolveGroup(910001, 83)
    api.updateGroupAnnouncement(910001, 83, "hello")
    api.updateGroupIcon(910001, 83, "https://cdn.example/group.png")
    api.setGroupAdmin(910001, 83, "u323456789", true, 0)
    api.muteGroupMember(910001, 83, "u323456789", 60)

    expect(sent).toEqual([
      {
        reqId: ReqId.ID_SEARCH_USER_REQ,
        payload: { user_id: "u123456789" },
      },
      {
        reqId: ReqId.ID_ADD_FRIEND_REQ,
        payload: {
          uid: 910001,
          applyname: "Alice",
          bakname: "memo",
          touid: 910002,
          labels: ["classmate"],
        },
      },
      {
        reqId: ReqId.ID_AUTH_FRIEND_REQ,
        payload: {
          fromuid: 910001,
          touid: 910003,
          back: "Bob",
          labels: ["team"],
        },
      },
      {
        reqId: ReqId.ID_CREATE_GROUP_REQ,
        payload: {
          fromuid: 910001,
          name: "Runtime Team",
          member_limit: 200,
          member_user_ids: ["u123456789", "u223456789"],
        },
      },
      {
        reqId: ReqId.ID_INVITE_GROUP_MEMBER_REQ,
        payload: {
          fromuid: 910001,
          groupid: 83,
          target_user_id: "u323456789",
          reason: "join us",
        },
      },
      {
        reqId: ReqId.ID_KICK_GROUP_MEMBER_REQ,
        payload: {
          fromuid: 910001,
          groupid: 83,
          target_user_id: "u323456789",
        },
      },
      {
        reqId: ReqId.ID_QUIT_GROUP_REQ,
        payload: {
          fromuid: 910001,
          groupid: 83,
        },
      },
      {
        reqId: ReqId.ID_DISSOLVE_GROUP_REQ,
        payload: {
          fromuid: 910001,
          groupid: 83,
        },
      },
      {
        reqId: ReqId.ID_UPDATE_GROUP_ANNOUNCEMENT_REQ,
        payload: {
          fromuid: 910001,
          groupid: 83,
          announcement: "hello",
        },
      },
      {
        reqId: ReqId.ID_UPDATE_GROUP_ICON_REQ,
        payload: {
          fromuid: 910001,
          groupid: 83,
          icon: "https://cdn.example/group.png",
        },
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
        payload: {
          fromuid: 910001,
          groupid: 83,
          target_user_id: "u323456789",
          mute_seconds: 60,
        },
      },
    ])
  })

  it("posts smart chat feature requests through the AI smart endpoint", async () => {
    const post = vi.fn().mockResolvedValue({ code: 0, result: "摘要内容" })
    const transport: ChatTransport = {
      connect: () => undefined,
      close: () => undefined,
      isConnected: () => true,
      send: () => undefined,
    }
    const api = createChatApi(transport, { post } as unknown as HttpClient)

    await expect(api.runSmartFeature(910001, "summary", "[{\"content\":\"hello\"}]", {
      context: { dialog: "user:910002", max_messages: 100 },
    })).resolves.toEqual({ code: 0, result: "摘要内容" })

    expect(post).toHaveBeenCalledWith("/ai/smart", {
      uid: 910001,
      feature_type: "summary",
      content: "[{\"content\":\"hello\"}]",
      model_type: "",
      model_name: "",
      deployment_preference: "any",
      context_json: JSON.stringify({ dialog: "user:910002", max_messages: 100 }),
    })
  })

  it("sends private and group messages in the ChatServer wire format", () => {
    const sent: Array<{ reqId: ReqId; payload: unknown }> = []
    const transport: ChatTransport = {
      connect: () => undefined,
      close: () => undefined,
      isConnected: () => true,
      send: (reqId, payload) => sent.push({ reqId, payload: JSON.parse(payload) as unknown }),
    }

    const api = createChatApi(transport, {} as HttpClient)
    const before = Date.now()
    const privateMsgId = api.sendPrivateMessage(910001, 910002, "hello")
    const groupMsgId = api.sendGroupMessage(910001, 83, "group hello")

    expect(sent).toHaveLength(2)
    expect(sent[0]).toMatchObject({
      reqId: ReqId.ID_TEXT_CHAT_MSG_REQ,
      payload: {
        fromuid: 910001,
        touid: 910002,
        text_array: [{
          msgid: privateMsgId,
          content: "hello",
        }],
      },
    })
    expect((sent[0]?.payload as { text_array: Array<{ created_at: number }> }).text_array[0]?.created_at)
      .toBeGreaterThanOrEqual(before)
    expect(sent[1]).toMatchObject({
      reqId: ReqId.ID_GROUP_CHAT_MSG_REQ,
      payload: {
        fromuid: 910001,
        groupid: 83,
        msg: {
          msgid: groupMsgId,
          content: "group hello",
          msgtype: "text",
        },
      },
    })
  })

  it("sends private and group history payloads in the ChatServer wire format", () => {
    const sent: Array<{ reqId: ReqId; payload: unknown }> = []
    const transport: ChatTransport = {
      connect: () => undefined,
      close: () => undefined,
      isConnected: () => true,
      send: (reqId, payload) => sent.push({ reqId, payload: JSON.parse(payload) as unknown }),
    }

    const api = createChatApi(transport, {} as HttpClient)
    api.fetchPrivateHistory(910001, 910002)
    api.fetchGroupHistory(910001, 83)

    expect(sent).toEqual([
      {
        reqId: ReqId.ID_PRIVATE_HISTORY_REQ,
        payload: {
          fromuid: 910001,
          peer_uid: 910002,
          before_ts: 0,
          limit: 50,
        },
      },
      {
        reqId: ReqId.ID_GROUP_HISTORY_REQ,
        payload: {
          fromuid: 910001,
          groupid: 83,
          before_ts: 0,
          before_seq: 0,
          limit: 50,
        },
      },
    ])
  })
})
