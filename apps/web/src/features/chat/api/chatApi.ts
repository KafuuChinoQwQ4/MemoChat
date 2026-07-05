/** Chat API — send/edit/revoke/forward private + group messages, dialog list */
import type { HttpClient } from "@/core/network/http/HttpClient"
import type { ChatTransport } from "@/core/network/transport/ChatTransport"
import { ReqId } from "@/core/network/opcodes/reqIds"
import { genClientMsgId } from "@/shared/lib/id"

export function createChatApi(transport: ChatTransport, _http: HttpClient) {
  return {
    /** Send a private text message */
    sendPrivateMessage(toUid: number, content: string): string {
      const clientMsgId = genClientMsgId()
      transport.send(ReqId.ID_TEXT_CHAT_MSG_REQ, JSON.stringify({
        to_uid: toUid,
        content,
        client_msg_id: clientMsgId,
        msg_type: "text",
      }))
      return clientMsgId
    },

    /** Send a group text message */
    sendGroupMessage(groupId: number, content: string): string {
      const clientMsgId = genClientMsgId()
      transport.send(ReqId.ID_GROUP_CHAT_MSG_REQ, JSON.stringify({
        group_id: groupId,
        content,
        client_msg_id: clientMsgId,
        msg_type: "text",
      }))
      return clientMsgId
    },

    /** Fetch private chat history */
    fetchPrivateHistory(peerUid: number, beforeMsgId: number, count = 13) {
      transport.send(ReqId.ID_PRIVATE_HISTORY_REQ, JSON.stringify({
        peer_uid: peerUid,
        before_msg_id: beforeMsgId,
        count,
      }))
    },

    /** Fetch dialog list */
    fetchDialogList(page = 1) {
      transport.send(ReqId.ID_GET_DIALOG_LIST_REQ, JSON.stringify({ page }))
    },

    /** Send read ack for private conversation */
    sendReadAck(peerUid: number, msgId: number) {
      transport.send(ReqId.ID_PRIVATE_READ_ACK_REQ, JSON.stringify({
        peer_uid: peerUid,
        last_read_msg_id: msgId,
      }))
    },

    /** Revoke a private message */
    revokePrivateMessage(peerUid: number, msgId: number) {
      transport.send(ReqId.ID_REVOKE_PRIVATE_MSG_REQ, JSON.stringify({ peer_uid: peerUid, msg_id: msgId }))
    },
  }
}

export type ChatApi = ReturnType<typeof createChatApi>
