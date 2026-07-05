import { useEntityStore } from "@/core/entities/entityStore";
import { useSessionStore } from "@/core/session/sessionStore";
import { uploadMedia } from "@/shared/media/MediaUploadService";
import { resolveMediaUrl } from "@/shared/media/mediaUrl";
import { ReqId } from "@/core/network/opcodes/reqIds";
export function bindChatPorts(container) {
    const { gateway } = container;
    return {
        session: {
            currentUserUid: () => useSessionStore.getState().uid ?? 0,
            isTransportReady: () => useSessionStore.getState().isReady(),
            getLoginTicket: () => useSessionStore.getState().loginTicket ?? "",
        },
        entities: {
            getFriend: (uid) => useEntityStore.getState().getFriend(uid),
            getConversationMessages: (peerId) => useEntityStore.getState().getMessages(peerId),
            getDialogList: () => useEntityStore.getState().getDialogList(),
        },
        media: {
            uploadAttachment: (req) => uploadMedia(req),
            resolveMediaUrl: (ref) => resolveMediaUrl(ref),
        },
        transport: {
            send: (reqId, payload) => gateway.chatTransport.send(reqId, payload),
        },
        onIncoming: {
            subscribePrivateMsg: (cb) => gateway.dispatcher.subscribe(ReqId.ID_NOTIFY_TEXT_CHAT_MSG_REQ, (f) => cb(JSON.parse(f.payload))),
            subscribeGroupMsg: (cb) => gateway.dispatcher.subscribe(ReqId.ID_NOTIFY_GROUP_CHAT_MSG_REQ, (f) => cb(JSON.parse(f.payload))),
            subscribeReadAck: (cb) => gateway.dispatcher.subscribe(ReqId.ID_NOTIFY_PRIVATE_READ_ACK_REQ, (f) => cb(JSON.parse(f.payload))),
        },
        navigation: {
            jumpToConversation: (uid) => {
                window.location.hash = `#/app/chat?peer=${uid}`;
            },
        },
    };
}
