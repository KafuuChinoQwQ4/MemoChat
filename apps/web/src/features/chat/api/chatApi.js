import { ReqId } from "@/core/network/opcodes/reqIds";
import { ENDPOINTS } from "@/core/config/endpoints";
import { genClientMsgId } from "@/shared/lib/id";
export function createChatApi(transport, _http) {
    return {
        /** Search a user by public user id, e.g. u123456789 */
        searchUser(userId) {
            transport.send(ReqId.ID_SEARCH_USER_REQ, JSON.stringify({
                user_id: userId.trim(),
            }));
        },
        /** Send a friend apply request */
        sendAddFriendApply(fromUid, applyName, targetUid, remark = "", labels = []) {
            const cleanName = applyName.trim() || "未知用户";
            transport.send(ReqId.ID_ADD_FRIEND_REQ, JSON.stringify({
                uid: fromUid,
                applyname: cleanName,
                bakname: remark.trim() || cleanName,
                touid: targetUid,
                labels: labels.map((item) => item.trim()).filter(Boolean),
            }));
        },
        /** Approve an incoming friend apply */
        approveFriendApply(fromUid, targetUid, remark = "", labels = []) {
            transport.send(ReqId.ID_AUTH_FRIEND_REQ, JSON.stringify({
                fromuid: fromUid,
                touid: targetUid,
                back: remark.trim(),
                labels: labels.map((item) => item.trim()).filter(Boolean),
            }));
        },
        /** Create a group using public user ids, matching the desktop client payload */
        createGroup(fromUid, name, memberUserIds, memberLimit = 200) {
            transport.send(ReqId.ID_CREATE_GROUP_REQ, JSON.stringify({
                fromuid: fromUid,
                name: name.trim(),
                member_limit: memberLimit,
                member_user_ids: memberUserIds.map((item) => item.trim()).filter(Boolean),
            }));
        },
        /** Fetch group list */
        fetchGroupList(fromUid) {
            transport.send(ReqId.ID_GET_GROUP_LIST_REQ, JSON.stringify({ fromuid: fromUid }));
        },
        /** Run desktop-equivalent smart chat features through the AI gateway */
        async runSmartFeature(uid, featureType, content, options = {}) {
            const payload = {
                uid,
                feature_type: featureType,
                content,
                model_type: options.modelType ?? "",
                model_name: options.modelName ?? "",
                deployment_preference: "any",
                context_json: JSON.stringify(options.context ?? {}),
            };
            if (featureType === "translate") {
                payload["target_lang"] = options.targetLang?.trim() || "中文";
            }
            return _http.post(ENDPOINTS.aiSmart, payload);
        },
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
