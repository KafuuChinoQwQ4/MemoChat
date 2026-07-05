import { ReqId } from "@/core/network/opcodes/reqIds";
import { useEntityStore } from "@/core/entities/entityStore";
import { useSessionStore } from "@/core/session/sessionStore";
import { genClientMsgId } from "@/shared/lib/id";
import { persistMessage } from "@/core/entities/persistence/chatDb";
export function registerChatRoutes(dispatcher) {
    const unsubs = [];
    // Incoming private message
    unsubs.push(dispatcher.subscribe(ReqId.ID_NOTIFY_TEXT_CHAT_MSG_REQ, (frame) => {
        const data = JSON.parse(frame.payload);
        const ownerUid = useSessionStore.getState().uid ?? 0;
        const peerId = data.from_uid === ownerUid ? data.to_uid : data.from_uid;
        const msg = {
            clientMsgId: genClientMsgId(),
            serverMsgId: data.msg_id,
            fromUid: data.from_uid,
            toId: peerId,
            isGroup: false,
            content: data.content,
            msgType: "text",
            timestamp: data.timestamp,
            state: "received",
        };
        useEntityStore.getState().appendMessage(peerId, msg);
        void persistMessage(ownerUid, peerId, msg);
    }));
    // Private chat ack (own message sent)
    unsubs.push(dispatcher.subscribe(ReqId.ID_TEXT_CHAT_MSG_RSP, (frame) => {
        const data = JSON.parse(frame.payload);
        if (data.error === 0 && data.client_msg_id) {
            useEntityStore.getState().updateMessageState(data.to_uid, data.client_msg_id, "sent", data.msg_id);
        }
    }));
    // Incoming group message
    unsubs.push(dispatcher.subscribe(ReqId.ID_NOTIFY_GROUP_CHAT_MSG_REQ, (frame) => {
        const data = JSON.parse(frame.payload);
        const ownerUid = useSessionStore.getState().uid ?? 0;
        const msg = {
            clientMsgId: genClientMsgId(),
            serverMsgId: data.msg_id,
            fromUid: data.from_uid,
            toId: data.group_id,
            isGroup: true,
            content: data.content,
            msgType: "text",
            timestamp: data.timestamp,
            state: "received",
        };
        useEntityStore.getState().appendMessage(data.group_id, msg);
        void persistMessage(ownerUid, data.group_id, msg);
    }));
    // Group chat ack
    unsubs.push(dispatcher.subscribe(ReqId.ID_GROUP_CHAT_MSG_RSP, (frame) => {
        const data = JSON.parse(frame.payload);
        if (data.error === 0 && data.client_msg_id) {
            useEntityStore.getState().updateMessageState(data.group_id, data.client_msg_id, "sent", data.msg_id);
        }
    }));
    // Private message edit
    unsubs.push(dispatcher.subscribe(ReqId.ID_NOTIFY_PRIVATE_MSG_CHANGED_REQ, (frame) => {
        const data = JSON.parse(frame.payload);
        if (data.is_revoke) {
            useEntityStore.getState().revokeMessage(data.peer_uid, data.msg_id);
        }
        else if (data.content && data.edited_at) {
            useEntityStore.getState().patchMessageContent(data.peer_uid, data.msg_id, data.content, data.edited_at);
        }
    }));
    // Friend added notification
    unsubs.push(dispatcher.subscribe(ReqId.ID_NOTIFY_AUTH_FRIEND_REQ, (frame) => {
        const data = JSON.parse(frame.payload);
        useEntityStore.getState().upsertFriend({
            uid: data.uid, name: data.name, email: data.email, icon: data.icon,
        });
    }));
    return () => unsubs.forEach((u) => u());
}
