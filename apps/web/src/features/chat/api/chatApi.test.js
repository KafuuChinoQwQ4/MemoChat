import { describe, expect, it, vi } from "vitest";
import { ReqId } from "@/core/network/opcodes/reqIds";
import { createChatApi } from "./chatApi";
describe("chatApi history requests", () => {
    it("sends relation and group management payloads in the desktop wire format", () => {
        const sent = [];
        const transport = {
            connect: () => undefined,
            close: () => undefined,
            isConnected: () => true,
            send: (reqId, payload) => sent.push({ reqId, payload: JSON.parse(payload) }),
        };
        const api = createChatApi(transport, {});
        api.searchUser(" u123456789 ");
        api.sendAddFriendApply(910001, "Alice", 910002, "memo", ["classmate", ""]);
        api.approveFriendApply(910001, 910003, "Bob", ["team"]);
        api.createGroup(910001, " Runtime Team ", ["u123456789", " u223456789 "]);
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
        ]);
    });
    it("posts smart chat feature requests through the AI smart endpoint", async () => {
        const post = vi.fn().mockResolvedValue({ code: 0, result: "摘要内容" });
        const transport = {
            connect: () => undefined,
            close: () => undefined,
            isConnected: () => true,
            send: () => undefined,
        };
        const api = createChatApi(transport, { post });
        await expect(api.runSmartFeature(910001, "summary", "[{\"content\":\"hello\"}]", {
            context: { dialog: "user:910002", max_messages: 100 },
        })).resolves.toEqual({ code: 0, result: "摘要内容" });
        expect(post).toHaveBeenCalledWith("/ai/smart", {
            uid: 910001,
            feature_type: "summary",
            content: "[{\"content\":\"hello\"}]",
            model_type: "",
            model_name: "",
            deployment_preference: "any",
            context_json: JSON.stringify({ dialog: "user:910002", max_messages: 100 }),
        });
    });
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
