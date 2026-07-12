import { beforeEach, describe, expect, it } from "vitest"
import { ChatMessageDispatcher } from "@/core/network/dispatcher/ChatMessageDispatcher"
import { ReqId } from "@/core/network/opcodes/reqIds"
import { useEntityStore } from "@/core/entities/entityStore"
import { useSessionStore } from "@/core/session/sessionStore"
import { registerChatRoutes } from "./registerChatRoutes"

describe("registerChatRoutes list bootstraps", () => {
  beforeEach(() => {
    useEntityStore.getState().reset()
    useSessionStore.getState().clearSession()
    useSessionStore.getState().setLogin({
      uid: 910001,
      token: "token",
      loginTicket: "ticket",
      ticketExpireMs: Date.now() + 60_000,
      protocolVersion: 3,
      chatEndpoints: [],
      profile: { uid: 910001, name: "RuntimeSmokeA", email: "a@example.test", icon: "" },
    })
  })

  it("stores group-list responses for the group page", () => {
    const dispatcher = new ChatMessageDispatcher()
    const unregister = registerChatRoutes(dispatcher)

    dispatcher.dispatch({
      reqId: ReqId.ID_GET_GROUP_LIST_RSP,
      length: 0,
      payload: JSON.stringify({
        error: 0,
        group_list: [{
          groupid: 83,
          group_code: "g123456789",
          name: "Runtime Smoke Full Chat",
          icon: "group.png",
          member_count: 2,
          announcement: "ship",
          role: 3,
        }],
      }),
    })

    expect(useEntityStore.getState().groups.get(83)).toMatchObject({
      groupId: 83,
      groupCode: "g123456789",
      name: "Runtime Smoke Full Chat",
      icon: "group.png",
      memberCount: 2,
      announcement: "ship",
      role: "owner",
    })

    unregister()
  })

  it("stores dialog-list responses for the chat page", () => {
    const dispatcher = new ChatMessageDispatcher()
    const unregister = registerChatRoutes(dispatcher)

    dispatcher.dispatch({
      reqId: ReqId.ID_GET_DIALOG_LIST_RSP,
      length: 0,
      payload: JSON.stringify({
        error: 0,
        dialogs: [
          {
            dialog_id: "u_910002",
            dialog_type: "private",
            peer_uid: 910002,
            title: "RuntimeSmokeB",
            avatar: "b.png",
            last_msg_preview: "[文件]",
            last_msg_ts: 1783310698641,
            unread_count: 2,
            pinned_rank: 9,
            draft_text: "later",
          },
          {
            dialog_id: "g_83",
            dialog_type: "group",
            group_id: 83,
            title: "Runtime Smoke Full Chat",
            last_msg_preview: "[图片]",
            last_msg_ts: 1783310696883,
            unread_count: 0,
          },
        ],
      }),
    })

    expect(useEntityStore.getState().dialogs.get(910002)).toMatchObject({
      dialogId: "u_910002",
      peerId: 910002,
      isGroup: false,
      title: "RuntimeSmokeB",
      avatar: "b.png",
      lastMsgContent: "[文件]",
      unreadCount: 2,
      isPinned: true,
      draftText: "later",
    })
    expect(useEntityStore.getState().dialogs.get(83)).toMatchObject({
      dialogId: "g_83",
      peerId: 83,
      isGroup: true,
      title: "Runtime Smoke Full Chat",
      lastMsgContent: "[图片]",
      unreadCount: 0,
      isPinned: false,
    })

    unregister()
  })

  it("stores private history responses as renderable conversation messages", () => {
    const dispatcher = new ChatMessageDispatcher()
    const unregister = registerChatRoutes(dispatcher)

    dispatcher.dispatch({
      reqId: ReqId.ID_PRIVATE_HISTORY_RSP,
      length: 0,
      payload: JSON.stringify({
        error: 0,
        peer_uid: 910002,
        has_more: false,
        messages: [
          {
            msgid: "p-1",
            fromuid: 910002,
            touid: 910001,
            content: "hello",
            created_at: 1783310698641,
          },
          {
            msgid: "p-2",
            fromuid: 910001,
            touid: 910002,
            content: "world",
            created_at: 1783310699641,
          },
        ],
      }),
    })

    expect(useEntityStore.getState().messages.get(910002)).toEqual([
      expect.objectContaining({
        clientMsgId: "p-1",
        fromUid: 910002,
        toId: 910002,
        isGroup: false,
        content: "hello",
        timestamp: 1783310698641,
        state: "received",
      }),
      expect.objectContaining({
        clientMsgId: "p-2",
        fromUid: 910001,
        toId: 910002,
        isGroup: false,
        content: "world",
        timestamp: 1783310699641,
        state: "sent",
      }),
    ])

    unregister()
  })

  it("stores group history responses as renderable conversation messages", () => {
    const dispatcher = new ChatMessageDispatcher()
    const unregister = registerChatRoutes(dispatcher)

    dispatcher.dispatch({
      reqId: ReqId.ID_GROUP_HISTORY_RSP,
      length: 0,
      payload: JSON.stringify({
        error: 0,
        groupid: 83,
        has_more: false,
        next_before_seq: 0,
        messages: [
          {
            msgid: "g-7",
            groupid: 83,
            fromuid: 910002,
            msgtype: "text",
            content: "group hello",
            created_at: 1783310700000,
            server_msg_id: 7001,
            group_seq: 7,
          },
        ],
      }),
    })

    expect(useEntityStore.getState().messages.get(83)).toEqual([
      expect.objectContaining({
        clientMsgId: "g-7",
        serverMsgId: 7001,
        fromUid: 910002,
        toId: 83,
        isGroup: true,
        content: "group hello",
        timestamp: 1783310700000,
        state: "received",
      }),
    ])

    unregister()
  })

  it("stores current group notify payloads with nested msg objects", () => {
    const dispatcher = new ChatMessageDispatcher()
    const unregister = registerChatRoutes(dispatcher)

    dispatcher.dispatch({
      reqId: ReqId.ID_NOTIFY_GROUP_CHAT_MSG_REQ,
      length: 0,
      payload: JSON.stringify({
        error: 0,
        fromuid: 910002,
        groupid: 83,
        client_msg_id: "g-live-1",
        from_name: "RuntimeSmokeB",
        from_icon: "head_1",
        msg: {
          msgid: "g-live-1",
          content: "live group",
          msgtype: "text",
          created_at: 1783310701000,
          server_msg_id: 7002,
        },
        created_at: 1783310701000,
        server_msg_id: 7002,
      }),
    })

    expect(useEntityStore.getState().messages.get(83)).toEqual([
      expect.objectContaining({
        clientMsgId: "g-live-1",
        serverMsgId: 7002,
        fromUid: 910002,
        toId: 83,
        isGroup: true,
        content: "live group",
        timestamp: 1783310701000,
        state: "received",
        senderName: "RuntimeSmokeB",
        senderIcon: "head_1",
      }),
    ])

    unregister()
  })

  it("marks optimistic private messages sent when the ChatServer ACK arrives", () => {
    const dispatcher = new ChatMessageDispatcher()
    const unregister = registerChatRoutes(dispatcher)
    useEntityStore.getState().appendMessage(910002, {
      clientMsgId: "p-live-ack",
      fromUid: 910001,
      toId: 910002,
      isGroup: false,
      content: "ack me",
      msgType: "text",
      timestamp: 1783310702000,
      state: "sending",
    })

    dispatcher.dispatch({
      reqId: ReqId.ID_TEXT_CHAT_MSG_RSP,
      length: 0,
      payload: JSON.stringify({
        error: 0,
        fromuid: 910001,
        touid: 910002,
        client_msg_id: "p-live-ack",
        status: "persisted",
        text_array: [{
          msgid: "p-live-ack",
          content: "ack me",
          created_at: 1783310702000,
        }],
      }),
    })

    expect(useEntityStore.getState().messages.get(910002)).toEqual([
      expect.objectContaining({
        clientMsgId: "p-live-ack",
        state: "sent",
      }),
    ])

    unregister()
  })

  it("marks optimistic private messages failed when the ChatServer ACK rejects them", () => {
    const dispatcher = new ChatMessageDispatcher()
    const unregister = registerChatRoutes(dispatcher)
    useEntityStore.getState().appendMessage(910002, {
      clientMsgId: "p-live-fail",
      fromUid: 910001,
      toId: 910002,
      isGroup: false,
      content: "fail me",
      msgType: "text",
      timestamp: 1783310703000,
      state: "sending",
    })

    dispatcher.dispatch({
      reqId: ReqId.ID_TEXT_CHAT_MSG_RSP,
      length: 0,
      payload: JSON.stringify({
        error: 1005,
        fromuid: 910001,
        touid: 910002,
        text_array: [{
          msgid: "p-live-fail",
          content: "fail me",
          created_at: 1783310703000,
        }],
      }),
    })

    expect(useEntityStore.getState().messages.get(910002)).toEqual([
      expect.objectContaining({
        clientMsgId: "p-live-fail",
        state: "failed",
      }),
    ])

    unregister()
  })

  it("marks optimistic group messages sent from nested ACK message ids", () => {
    const dispatcher = new ChatMessageDispatcher()
    const unregister = registerChatRoutes(dispatcher)
    useEntityStore.getState().appendMessage(83, {
      clientMsgId: "g-live-ack",
      fromUid: 910001,
      toId: 83,
      isGroup: true,
      content: "group ack",
      msgType: "text",
      timestamp: 1783310704000,
      state: "sending",
    })

    dispatcher.dispatch({
      reqId: ReqId.ID_GROUP_CHAT_MSG_RSP,
      length: 0,
      payload: JSON.stringify({
        error: 0,
        fromuid: 910001,
        groupid: 83,
        msg: {
          msgid: "g-live-ack",
          content: "group ack",
          msgtype: "text",
          created_at: 1783310704000,
          server_msg_id: 8001,
        },
        server_msg_id: 8001,
      }),
    })

    expect(useEntityStore.getState().messages.get(83)).toEqual([
      expect.objectContaining({
        clientMsgId: "g-live-ack",
        serverMsgId: 8001,
        state: "sent",
      }),
    ])

    unregister()
  })
})
