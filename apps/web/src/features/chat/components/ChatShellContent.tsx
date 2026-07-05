/** ChatShellContent — full chat feature layout: dialog list + conversation pane */
import { useEffect } from "react"
import { useSearchParams } from "react-router-dom"
import { DialogListPanel } from "./sidebar/DialogListPanel"
import { ConversationPane } from "./conversation/ConversationPane"
import { useChatStore } from "@/features/chat/store/chatStore"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"

export function ChatShellContent() {
  const [searchParams] = useSearchParams()
  const { selectedPeerId, setSelectedConversation } = useChatStore()

  // Handle ?peer=uid navigation from ports.navigation.jumpToConversation
  useEffect(() => {
    const peer = searchParams.get("peer")
    if (peer) setSelectedConversation(Number(peer), false)
  }, [searchParams, setSelectedConversation])

  return (
    <div style={{ display: "flex", height: "100%", width: "100%", overflow: "hidden" }}>
      {/* ~250px dialog list */}
      <div style={{ width: "var(--list-panel-w)", flexShrink: 0, borderRight: "1px solid var(--divider)", overflow: "hidden" }}>
        <DialogListPanel />
      </div>
      {/* Conversation area */}
      <div style={{ flex: 1, overflow: "hidden" }}>
        {selectedPeerId !== null ? (
          <ConversationPane peerId={selectedPeerId} />
        ) : (
          <GlassSurface style={{
            height: "100%",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            color: "var(--text-disabled)",
            fontSize: 14,
          }}>
            选择一个对话开始聊天
          </GlassSurface>
        )}
      </div>
    </div>
  )
}
