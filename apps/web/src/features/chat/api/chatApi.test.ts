import { describe, expect, it } from "vitest"
import { ReqId } from "@/core/network/opcodes/reqIds"
import type { ChatTransport } from "@/core/network/transport/ChatTransport"
import type { HttpClient } from "@/core/network/http/HttpClient"
import { createChatApi } from "./chatApi"

describe("chatApi history requests", () => {
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
