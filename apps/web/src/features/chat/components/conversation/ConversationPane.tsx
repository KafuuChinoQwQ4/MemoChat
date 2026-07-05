/**
 * ConversationPane — full conversation view: message history + composer.
 * Handles message sending via chatApi (optimistic) and reads from entityStore.
 */
import { useEffect, useRef, useMemo } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { useSessionStore } from "@/core/session/sessionStore"
import { useChatStore } from "@/features/chat/store/chatStore"
import { createChatApi } from "@/features/chat/api/chatApi"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { genClientMsgId } from "@/shared/lib/id"
import { MessageBubble } from "./MessageBubble"
import { ComposerBar } from "./ComposerBar"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import type { RichMessage } from "@/core/entities/entityTypes"

interface ConversationPaneProps {
  peerId: number
}

export function ConversationPane({ peerId }: ConversationPaneProps) {
  // Subscribe to the messages array for this peer directly — same stable-ref pattern.
  const messagesMap = useEntityStore((s) => s.messages)
  const messages = useMemo(() => messagesMap.get(peerId) ?? [], [messagesMap, peerId])
  const myUid = useSessionStore((s) => s.uid) ?? 0
  const { selectedIsGroup } = useChatStore()
  const gateway = getGateway()
  const api = createChatApi(gateway.chatTransport, gateway.http)
  const bottomRef = useRef<HTMLDivElement>(null)

  // Scroll to bottom on new messages
  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" })
  }, [messages.length])

  function handleSend(text: string) {
    const clientMsgId = selectedIsGroup
      ? api.sendGroupMessage(peerId, text)
      : api.sendPrivateMessage(peerId, text)

    // Optimistic append
    const msg: RichMessage = {
      clientMsgId,
      fromUid: myUid,
      toId: peerId,
      isGroup: selectedIsGroup,
      content: text,
      msgType: "text",
      timestamp: Date.now(),
      state: "sending",
    }
    useEntityStore.getState().appendMessage(peerId, msg)
  }

  return (
    <div style={{ height: "100%", display: "flex", flexDirection: "column", background: "rgba(255,255,255,0.03)" }}>
      {/* Message list */}
      <GlassScrollArea style={{ flex: 1, paddingTop: 12 }}>
        {messages.map((msg) => (
          <MessageBubble key={msg.clientMsgId} message={msg} />
        ))}
        <div ref={bottomRef} />
      </GlassScrollArea>

      {/* Composer */}
      <ComposerBar onSend={handleSend} />
    </div>
  )
}
