import { ReqId } from "@/core/network/opcodes/reqIds";
import { useEntityStore } from "@/core/entities/entityStore";
import { useSessionStore } from "@/core/session/sessionStore";
import { useChatStore } from "@/features/chat/store/chatStore";
import { genClientMsgId } from "@/shared/lib/id";
import { persistMessage } from "@/core/entities/persistence/chatDb";
import { applyDialogListPayload, applyGroupListPayload } from "./chatListPayloads";
function parseWireObject(payload) {
    try {
        const parsed = JSON.parse(payload);
        return parsed && typeof parsed === "object" && !Array.isArray(parsed)
            ? parsed
            : null;
    }
    catch {
        return null;
    }
}
function asWireObject(value) {
    return value && typeof value === "object" && !Array.isArray(value)
        ? value
        : null;
}
function numberValue(value) {
    if (typeof value === "number" && Number.isFinite(value))
        return value;
    if (typeof value === "string" && value.trim()) {
        const parsed = Number(value);
        if (Number.isFinite(parsed))
            return parsed;
    }
    return undefined;
}
function stringValue(value) {
    if (typeof value === "string")
        return value;
    if (typeof value === "number" && Number.isFinite(value))
        return String(value);
    return undefined;
}
function fieldNumber(row, ...keys) {
    for (const key of keys) {
        const value = numberValue(row[key]);
        if (value !== undefined)
            return value;
    }
    return 0;
}
function fieldString(row, ...keys) {
    for (const key of keys) {
        const value = stringValue(row[key]);
        if (value !== undefined)
            return value;
    }
    return "";
}
function fieldBoolean(row, key) {
    const value = row[key];
    return value === true || value === 1 || value === "1" || value === "true";
}
function normalizeTimestamp(value) {
    if (value <= 0)
        return Date.now();
    return value < 100000000000 ? value * 1000 : value;
}
function normalizeMsgType(value) {
    const msgType = value.toLowerCase();
    if (msgType === "image" || msgType === "file" || msgType === "system")
        return msgType;
    return "text";
}
function optionalServerMsgId(row) {
    return numberValue(row["server_msg_id"]) ?? numberValue(row["msg_id"]);
}
function payloadClientMsgId(row) {
    const rootId = fieldString(row, "client_msg_id", "msgid", "msg_id");
    if (rootId)
        return rootId;
    const rows = messageRows(row);
    for (const msgRow of rows) {
        const msgId = fieldString(msgRow, "client_msg_id", "msgid", "msg_id");
        if (msgId)
            return msgId;
    }
    return "";
}
function privatePeerId(row, ownerUid) {
    const explicitPeer = fieldNumber(row, "peer_uid", "peeruid");
    if (explicitPeer > 0)
        return explicitPeer;
    const fromUid = fieldNumber(row, "from_uid", "fromuid");
    const toUid = fieldNumber(row, "to_uid", "touid");
    if (fromUid === ownerUid && toUid > 0)
        return toUid;
    if (toUid === ownerUid && fromUid > 0)
        return fromUid;
    return toUid > 0 ? toUid : fromUid;
}
function messageRows(row) {
    const messages = row["messages"];
    if (Array.isArray(messages)) {
        return messages.map(asWireObject).filter((item) => item !== null);
    }
    const textArray = row["text_array"];
    if (Array.isArray(textArray)) {
        return textArray.map(asWireObject).filter((item) => item !== null);
    }
    const nestedMsg = asWireObject(row["msg"]);
    if (nestedMsg) {
        return [nestedMsg];
    }
    return [row];
}
function privateRichMessage(row, peerId, ownerUid) {
    const fromUid = fieldNumber(row, "from_uid", "fromuid");
    if (fromUid <= 0 || peerId <= 0)
        return null;
    const clientMsgId = fieldString(row, "client_msg_id", "msgid", "msg_id") || genClientMsgId();
    const msg = {
        clientMsgId,
        fromUid,
        toId: peerId,
        isGroup: false,
        content: fieldString(row, "content", "text"),
        msgType: normalizeMsgType(fieldString(row, "msg_type", "msgtype")),
        timestamp: normalizeTimestamp(fieldNumber(row, "timestamp", "created_at", "createdAt")),
        state: fromUid === ownerUid ? "sent" : "received",
    };
    const senderName = fieldString(row, "from_name", "from_nick", "sender_name");
    if (senderName)
        msg.senderName = senderName;
    const senderIcon = fieldString(row, "from_icon", "sender_icon");
    if (senderIcon)
        msg.senderIcon = senderIcon;
    const fromUserId = fieldString(row, "from_user_id", "fromUserId");
    if (fromUserId)
        msg.fromUserId = fromUserId;
    const serverMsgId = optionalServerMsgId(row);
    if (serverMsgId !== undefined)
        msg.serverMsgId = serverMsgId;
    if (fieldNumber(row, "deleted_at_ms", "deletedAtMs") > 0 || fieldBoolean(row, "is_revoke")) {
        msg.isRevoked = true;
    }
    const editedAt = fieldNumber(row, "edited_at_ms", "editedAtMs");
    if (editedAt > 0)
        msg.editedAt = normalizeTimestamp(editedAt);
    return msg;
}
function groupRichMessage(row, groupId, ownerUid) {
    const fromUid = fieldNumber(row, "from_uid", "fromuid");
    if (fromUid <= 0 || groupId <= 0)
        return null;
    const clientMsgId = fieldString(row, "client_msg_id", "msgid", "msg_id") || genClientMsgId();
    const msg = {
        clientMsgId,
        fromUid,
        toId: groupId,
        isGroup: true,
        content: fieldString(row, "content", "text"),
        msgType: normalizeMsgType(fieldString(row, "msg_type", "msgtype")),
        timestamp: normalizeTimestamp(fieldNumber(row, "timestamp", "created_at", "createdAt")),
        state: fromUid === ownerUid ? "sent" : "received",
    };
    const senderName = fieldString(row, "from_name", "from_nick", "sender_name");
    if (senderName)
        msg.senderName = senderName;
    const senderIcon = fieldString(row, "from_icon", "sender_icon");
    if (senderIcon)
        msg.senderIcon = senderIcon;
    const fromUserId = fieldString(row, "from_user_id", "fromUserId");
    if (fromUserId)
        msg.fromUserId = fromUserId;
    const serverMsgId = optionalServerMsgId(row);
    if (serverMsgId !== undefined)
        msg.serverMsgId = serverMsgId;
    if (fieldNumber(row, "deleted_at_ms", "deletedAtMs") > 0 || fieldBoolean(row, "is_revoke")) {
        msg.isRevoked = true;
    }
    const editedAt = fieldNumber(row, "edited_at_ms", "editedAtMs");
    if (editedAt > 0)
        msg.editedAt = normalizeTimestamp(editedAt);
    return msg;
}
function sortedMessages(messages) {
    return [...messages].sort((a, b) => {
        if (a.timestamp !== b.timestamp)
            return a.timestamp - b.timestamp;
        return a.clientMsgId.localeCompare(b.clientMsgId);
    });
}
function persistMessages(ownerUid, peerId, messages) {
    if (ownerUid <= 0 || peerId <= 0 || messages.length === 0)
        return;
    void Promise.all(messages.map((msg) => persistMessage(ownerUid, peerId, msg))).catch(() => undefined);
}
export function registerChatRoutes(dispatcher) {
    const unsubs = [];
    unsubs.push(dispatcher.subscribe(ReqId.ID_GET_GROUP_LIST_RSP, (frame) => {
        applyGroupListPayload(frame.payload);
    }));
    unsubs.push(dispatcher.subscribe(ReqId.ID_GET_DIALOG_LIST_RSP, (frame) => {
        applyDialogListPayload(frame.payload);
    }));
    // Incoming private message
    unsubs.push(dispatcher.subscribe(ReqId.ID_NOTIFY_TEXT_CHAT_MSG_REQ, (frame) => {
        const data = parseWireObject(frame.payload);
        if (!data || fieldNumber(data, "error") !== 0)
            return;
        const ownerUid = useSessionStore.getState().uid ?? 0;
        const fromUid = fieldNumber(data, "from_uid", "fromuid");
        const toUid = fieldNumber(data, "to_uid", "touid");
        const peerId = fromUid === ownerUid ? toUid : fromUid;
        const messages = messageRows(data)
            .map((row) => privateRichMessage({ ...data, ...row }, peerId, ownerUid))
            .filter((msg) => msg !== null);
        for (const msg of messages) {
            useEntityStore.getState().appendMessage(peerId, msg);
        }
        persistMessages(ownerUid, peerId, messages);
    }));
    // Private chat ack (own message sent)
    unsubs.push(dispatcher.subscribe(ReqId.ID_TEXT_CHAT_MSG_RSP, (frame) => {
        const data = parseWireObject(frame.payload);
        if (!data)
            return;
        const ownerUid = useSessionStore.getState().uid ?? 0;
        const clientMsgId = payloadClientMsgId(data);
        const peerId = privatePeerId(data, ownerUid);
        const serverMsgId = numberValue(data["server_msg_id"]) ?? numberValue(data["msg_id"]);
        if (clientMsgId && peerId > 0) {
            useEntityStore.getState().updateMessageState(peerId, clientMsgId, fieldNumber(data, "error") === 0 ? "sent" : "failed", serverMsgId);
        }
    }));
    // Private history
    unsubs.push(dispatcher.subscribe(ReqId.ID_PRIVATE_HISTORY_RSP, (frame) => {
        const data = parseWireObject(frame.payload);
        useChatStore.getState().setLoadingHistory(false);
        if (!data || fieldNumber(data, "error") !== 0)
            return;
        const ownerUid = useSessionStore.getState().uid ?? 0;
        const peerId = fieldNumber(data, "peer_uid", "peeruid");
        if (peerId <= 0)
            return;
        const messages = sortedMessages(messageRows(data)
            .map((row) => privateRichMessage(row, peerId, ownerUid))
            .filter((msg) => msg !== null));
        useEntityStore.getState().prependMessages(peerId, messages);
        useChatStore.getState().setHistoryFinished(!fieldBoolean(data, "has_more"));
        persistMessages(ownerUid, peerId, messages);
    }));
    // Incoming group message
    unsubs.push(dispatcher.subscribe(ReqId.ID_NOTIFY_GROUP_CHAT_MSG_REQ, (frame) => {
        const data = parseWireObject(frame.payload);
        if (!data || fieldNumber(data, "error") !== 0)
            return;
        const ownerUid = useSessionStore.getState().uid ?? 0;
        const groupId = fieldNumber(data, "group_id", "groupid");
        if (groupId <= 0)
            return;
        const messages = messageRows(data)
            .map((row) => groupRichMessage({ ...data, ...row }, groupId, ownerUid))
            .filter((msg) => msg !== null);
        for (const msg of messages) {
            useEntityStore.getState().appendMessage(groupId, msg);
        }
        persistMessages(ownerUid, groupId, messages);
    }));
    // Group chat ack
    unsubs.push(dispatcher.subscribe(ReqId.ID_GROUP_CHAT_MSG_RSP, (frame) => {
        const data = parseWireObject(frame.payload);
        if (!data)
            return;
        const clientMsgId = payloadClientMsgId(data);
        const groupId = fieldNumber(data, "group_id", "groupid");
        const serverMsgId = numberValue(data["server_msg_id"]) ?? numberValue(data["msg_id"]);
        if (clientMsgId && groupId > 0) {
            useEntityStore.getState().updateMessageState(groupId, clientMsgId, fieldNumber(data, "error") === 0 ? "sent" : "failed", serverMsgId);
        }
    }));
    // Group history
    unsubs.push(dispatcher.subscribe(ReqId.ID_GROUP_HISTORY_RSP, (frame) => {
        const data = parseWireObject(frame.payload);
        useChatStore.getState().setLoadingHistory(false);
        if (!data || fieldNumber(data, "error") !== 0)
            return;
        const ownerUid = useSessionStore.getState().uid ?? 0;
        const groupId = fieldNumber(data, "group_id", "groupid");
        if (groupId <= 0)
            return;
        const messages = sortedMessages(messageRows(data)
            .map((row) => groupRichMessage(row, groupId, ownerUid))
            .filter((msg) => msg !== null));
        useEntityStore.getState().prependMessages(groupId, messages);
        useChatStore.getState().setHistoryFinished(!fieldBoolean(data, "has_more"));
        persistMessages(ownerUid, groupId, messages);
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
