/** ChatPorts — injected dependencies for the chat feature (mirrors AppChatFeaturePortBinder) */
import type { RichMessage, Friend, DialogEntry } from "@/core/entities/entityTypes"
import type { ReqId } from "@/core/network/opcodes/reqIds"
import type { MediaUploadResult, UploadRequest } from "@/shared/media/uploadTypes"
import type { Unsubscribe } from "@/core/network/dispatcher/dispatcher.types"

export interface ChatPorts {
  session: {
    currentUserUid(): number
    isTransportReady(): boolean
    getLoginTicket(): string
  }
  entities: {
    getFriend(uid: number): Friend | undefined
    getConversationMessages(peerId: number): RichMessage[]
    getDialogList(): DialogEntry[]
  }
  media: {
    uploadAttachment(req: UploadRequest): Promise<MediaUploadResult>
    resolveMediaUrl(ref: string): string
  }
  transport: {
    send(reqId: ReqId, payload: string): void
  }
  onIncoming: {
    subscribePrivateMsg(cb: (data: Record<string, unknown>) => void): Unsubscribe
    subscribeGroupMsg(cb: (data: Record<string, unknown>) => void): Unsubscribe
    subscribeReadAck(cb: (data: Record<string, unknown>) => void): Unsubscribe
  }
  navigation: {
    jumpToConversation(uid: number): void
  }
}

let _ports: ChatPorts | null = null

export function setChatPorts(ports: ChatPorts) { _ports = ports }
export function getChatPorts(): ChatPorts {
  if (!_ports) throw new Error("ChatPorts not initialized")
  return _ports
}
