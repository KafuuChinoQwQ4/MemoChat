import { ReqId } from "@/core/network/opcodes/reqIds";
import { genClientMsgId } from "@/shared/lib/id";
export function createChatApi(transport, _http) {
    return {
        /** Send a private text message */
        sendPrivateMessage(toUid, content) {
            const clientMsgId = genClientMsgId();
            transport.send(ReqId.ID_TEXT_CHAT_MSG_REQ, JSON.stringify({
                to_uid: toUid,
                content,
                client_msg_id: clientMsgId,
                msg_type: "text",
            }));
            return clientMsgId;
        },
        /** Send a group text message */
        sendGroupMessage(groupId, content) {
            const clientMsgId = genClientMsgId();
            transport.send(ReqId.ID_GROUP_CHAT_MSG_REQ, JSON.stringify({
                group_id: groupId,
                content,
                client_msg_id: clientMsgId,
                msg_type: "text",
            }));
            return clientMsgId;
        },
        /** Fetch private chat history */
        fetchPrivateHistory(peerUid, beforeMsgId, count = 13) {
            transport.send(ReqId.ID_PRIVATE_HISTORY_REQ, JSON.stringify({
                peer_uid: peerUid,
                before_msg_id: beforeMsgId,
                count,
            }));
        },
        /** Fetch dialog list */
        fetchDialogList(page = 1) {
            transport.send(ReqId.ID_GET_DIALOG_LIST_REQ, JSON.stringify({ page }));
        },
        /** Send read ack for private conversation */
        sendReadAck(peerUid, msgId) {
            transport.send(ReqId.ID_PRIVATE_READ_ACK_REQ, JSON.stringify({
                peer_uid: peerUid,
                last_read_msg_id: msgId,
            }));
        },
        /** Revoke a private message */
        revokePrivateMessage(peerUid, msgId) {
            transport.send(ReqId.ID_REVOKE_PRIVATE_MSG_REQ, JSON.stringify({ peer_uid: peerUid, msg_id: msgId }));
        },
    };
}
