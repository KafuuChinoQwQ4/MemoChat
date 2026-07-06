import { ReqId } from "@/core/network/opcodes/reqIds";
import { genClientMsgId } from "@/shared/lib/id";
export function createChatApi(transport, _http) {
    return {
        /** Send a private text message */
        sendPrivateMessage(fromUid, toUid, content) {
            const clientMsgId = genClientMsgId();
            const createdAt = Date.now();
            transport.send(ReqId.ID_TEXT_CHAT_MSG_REQ, JSON.stringify({
                fromuid: fromUid,
                touid: toUid,
                text_array: [{
                        msgid: clientMsgId,
                        content,
                        created_at: createdAt,
                    }],
            }));
            return clientMsgId;
        },
        /** Send a group text message */
        sendGroupMessage(fromUid, groupId, content) {
            const clientMsgId = genClientMsgId();
            transport.send(ReqId.ID_GROUP_CHAT_MSG_REQ, JSON.stringify({
                fromuid: fromUid,
                groupid: groupId,
                msg: {
                    msgid: clientMsgId,
                    content,
                    msgtype: "text",
                },
            }));
            return clientMsgId;
        },
        /** Fetch private chat history */
        fetchPrivateHistory(fromUid, peerUid, beforeTs = 0, beforeMsgId = "", limit = 50) {
            const payload = {
                fromuid: fromUid,
                peer_uid: peerUid,
                before_ts: beforeTs,
                limit,
            };
            if (beforeMsgId.trim())
                payload["before_msg_id"] = beforeMsgId.trim();
            transport.send(ReqId.ID_PRIVATE_HISTORY_REQ, JSON.stringify(payload));
        },
        /** Fetch group chat history */
        fetchGroupHistory(fromUid, groupId, beforeSeq = 0, limit = 50) {
            transport.send(ReqId.ID_GROUP_HISTORY_REQ, JSON.stringify({
                fromuid: fromUid,
                groupid: groupId,
                before_ts: 0,
                before_seq: beforeSeq,
                limit,
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
