import { describe, expect, it } from "vitest";
import { ReqId } from "@/core/network/opcodes/reqIds";
import { createChatApi } from "./chatApi";
describe("chatApi history requests", () => {
    it("sends private and group messages in the ChatServer wire format", () => {
        const sent = [];
        const transport = {
            connect: () => undefined,
            close: () => undefined,
            isConnected: () => true,
            send: (reqId, payload) => sent.push({ reqId, payload: JSON.parse(payload) }),
        };
        const api = createChatApi(transport, {});
        const before = Date.now();
        const privateMsgId = api.sendPrivateMessage(910001, 910002, "hello");
        const groupMsgId = api.sendGroupMessage(910001, 83, "group hello");
        expect(sent).toHaveLength(2);
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
        });
        expect((sent[0]?.payload).text_array[0]?.created_at)
            .toBeGreaterThanOrEqual(before);
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
        });
    });
    it("sends private and group history payloads in the ChatServer wire format", () => {
        const sent = [];
        const transport = {
            connect: () => undefined,
            close: () => undefined,
            isConnected: () => true,
            send: (reqId, payload) => sent.push({ reqId, payload: JSON.parse(payload) }),
        };
        const api = createChatApi(transport, {});
        api.fetchPrivateHistory(910001, 910002);
        api.fetchGroupHistory(910001, 83);
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
        ]);
    });
});
