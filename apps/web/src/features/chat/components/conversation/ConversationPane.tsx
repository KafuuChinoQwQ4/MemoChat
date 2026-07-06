/**
 * ConversationPane — full conversation view: message history + composer.
 * Handles message sending via chatApi (optimistic) and reads from entityStore.
 */
import { useEffect, useRef, useMemo, useState } from "react"
import { useEntityStore } from "@/core/entities/entityStore"
import { useSessionStore } from "@/core/session/sessionStore"
import { displayNameWithoutInternalId, groupPublicIdText } from "@/core/entities/displayIds"
import { useChatStore } from "@/features/chat/store/chatStore"
import { createChatApi } from "@/features/chat/api/chatApi"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { hydrateConversation } from "@/core/entities/persistence/messagePersistence"
import { MessageBubble } from "./MessageBubble"
import { ComposerBar } from "./ComposerBar"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import type { RichMessage } from "@/core/entities/entityTypes"

interface ConversationPaneProps {
  peerId: number
}

const AVATAR_STACK_WINDOW_MS = 60_000
const TIME_SEPARATOR_WINDOW_MS = 5 * 60_000

function shouldShowMessageAvatar(previous: RichMessage | undefined, current: RichMessage): boolean {
  if (!previous) return true
  if (previous.fromUid !== current.fromUid) return true
  if (previous.timestamp <= 0 || current.timestamp <= 0) return true
  return current.timestamp - previous.timestamp > AVATAR_STACK_WINDOW_MS
}

function shouldShowTimeSeparator(previous: RichMessage | undefined, current: RichMessage): boolean {
  if (!previous) return true
  if (previous.timestamp <= 0 || current.timestamp <= 0) return false
  const previousDate = new Date(previous.timestamp)
  const currentDate = new Date(current.timestamp)
  if (previousDate.toDateString() !== currentDate.toDateString()) return true
  return current.timestamp - previous.timestamp > TIME_SEPARATOR_WINDOW_MS
}

function formatConversationTime(timestamp: number): string {
  if (timestamp <= 0) return ""
  const date = new Date(timestamp)
  const now = new Date()
  const time = date.toLocaleTimeString("zh-CN", { hour: "2-digit", minute: "2-digit", hour12: false })
  if (date.toDateString() === now.toDateString()) return time

  const yesterday = new Date(now)
  yesterday.setDate(now.getDate() - 1)
  if (date.toDateString() === yesterday.toDateString()) return `昨天 ${time}`

  const diffDays = Math.floor((startOfDay(now).getTime() - startOfDay(date).getTime()) / 86_400_000)
  if (diffDays > 0 && diffDays < 7) {
    const weekday = date.toLocaleDateString("zh-CN", { weekday: "short" })
    return `${weekday} ${time}`
  }

  const day = date.toLocaleDateString("zh-CN", { month: "numeric", day: "numeric" })
  return `${day} ${time}`
}

function startOfDay(value: Date): Date {
  return new Date(value.getFullYear(), value.getMonth(), value.getDate())
}

function smartHistoryContent(messages: RichMessage[], myUid: number): string {
  const rows = messages
    .filter((msg) => msg.msgType === "text" && !msg.isRevoked && msg.content.trim())
    .slice(-100)
    .map((msg) => ({
      role: msg.fromUid === myUid ? "me" : "peer",
      from_uid: msg.fromUid,
      content: msg.content,
      timestamp: msg.timestamp,
    }))
  return JSON.stringify(rows)
}

function latestTranslatableMessage(messages: RichMessage[], myUid: number): RichMessage | null {
  const textMessages = messages.filter((msg) => msg.msgType === "text" && !msg.isRevoked && msg.content.trim())
  return [...textMessages].reverse().find((msg) => msg.fromUid !== myUid) ?? textMessages[textMessages.length - 1] ?? null
}

function groupTitleWithoutInternalId(name: string | undefined, publicId: string, peerId: number): string {
  const trimmed = name?.trim() ?? ""
  if (trimmed && trimmed !== String(peerId) && trimmed !== `群聊 ${peerId}`) return trimmed
  return publicId === "未分配群号" ? "群聊" : `群聊 ${publicId}`
}

export function ConversationPane({ peerId }: ConversationPaneProps) {
  // Subscribe to the messages array for this peer directly — same stable-ref pattern.
  const messagesMap = useEntityStore((s) => s.messages)
  const friendsMap = useEntityStore((s) => s.friends)
  const groupsMap = useEntityStore((s) => s.groups)
  const dialogsMap = useEntityStore((s) => s.dialogs)
  const messages = useMemo(() => messagesMap.get(peerId) ?? [], [messagesMap, peerId])
  const myUid = useSessionStore((s) => s.uid) ?? 0
  const { selectedIsGroup } = useChatStore()
  const gateway = useMemo(() => getGateway(), [])
  const api = useMemo(() => createChatApi(gateway.chatTransport, gateway.http), [gateway])
  const bottomRef = useRef<HTMLDivElement>(null)
  const [smartBusy, setSmartBusy] = useState(false)
  const [smartTitle, setSmartTitle] = useState("")
  const [smartResult, setSmartResult] = useState("")
  const [smartError, setSmartError] = useState("")
  const peerTitle = useMemo(() => {
    const dialogTitle = dialogsMap.get(peerId)?.title?.trim()
    if (selectedIsGroup) {
      const group = groupsMap.get(peerId)
      return groupTitleWithoutInternalId(group?.name || dialogTitle, groupPublicIdText(group?.groupCode), peerId)
    }
    const friend = friendsMap.get(peerId)
    return displayNameWithoutInternalId(friend?.name || dialogTitle, friend?.userId, peerId, "未知用户")
  }, [dialogsMap, friendsMap, groupsMap, peerId, selectedIsGroup])

  useEffect(() => {
    let cancelled = false
    if (myUid <= 0 || peerId <= 0) return () => { cancelled = true }

    useChatStore.getState().setLoadingHistory(true)
    void hydrateConversation(myUid, peerId)
      .catch((err: unknown) => {
        console.warn("[ConversationPane] hydrate failed", err)
      })
      .finally(() => {
        if (cancelled) return
        if (selectedIsGroup) {
          api.fetchGroupHistory(myUid, peerId)
        } else {
          api.fetchPrivateHistory(myUid, peerId)
        }
      })

    return () => { cancelled = true }
  }, [api, myUid, peerId, selectedIsGroup])

  // Scroll to bottom on new messages
  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" })
  }, [messages.length])

  function handleSend(text: string) {
    if (myUid <= 0) return
    const clientMsgId = selectedIsGroup
      ? api.sendGroupMessage(myUid, peerId, text)
      : api.sendPrivateMessage(myUid, peerId, text)

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

  async function runSmartAction(kind: "summary" | "suggest" | "translate") {
    if (myUid <= 0) {
      setSmartError("登录状态无效，请重新登录")
      return
    }
    const titleMap = {
      summary: "会话摘要",
      suggest: "建议回复",
      translate: "消息翻译",
    }
    let content = ""
    const context: Record<string, unknown> = {
      dialog: selectedIsGroup ? `group:${peerId}` : `user:${peerId}`,
      title: peerTitle,
    }
    if (kind === "translate") {
      const target = latestTranslatableMessage(messages, myUid)
      if (!target) {
        setSmartError("当前会话没有可翻译的文本消息")
        return
      }
      content = target.content
      context["source_lang"] = "自动检测"
      context["message_id"] = target.serverMsgId ?? target.clientMsgId
    } else {
      content = smartHistoryContent(messages, myUid)
      if (content === "[]") {
        setSmartError("当前会话还没有可分析的文本消息")
        return
      }
      context["max_messages"] = 100
      if (kind === "suggest") {
        context["format"] = "three_options"
      }
    }

    setSmartBusy(true)
    setSmartTitle(titleMap[kind])
    setSmartResult("")
    setSmartError("")
    try {
      const response = await api.runSmartFeature(myUid, kind, content, {
        targetLang: "中文",
        context,
      })
      const errorCode = response.error ?? response.code ?? 0
      if (errorCode !== 0) {
        setSmartError(response.message || `智能请求失败，错误码 ${errorCode}`)
        return
      }
      const result = (response.result || response.content || "").trim()
      setSmartResult(result || "没有返回内容")
    } catch (err) {
      setSmartError(err instanceof Error ? err.message : "智能请求失败")
    } finally {
      setSmartBusy(false)
    }
  }

  return (
    <div
      style={{
        height: "100%",
        display: "flex",
        flexDirection: "column",
        background: "linear-gradient(180deg, rgba(246,250,255,0.78), rgba(231,241,252,0.84))",
      }}
    >
      <div
        style={{
          minHeight: 44,
          padding: "0 14px",
          display: "flex",
          alignItems: "center",
          gap: 10,
          borderBottom: "1px solid rgba(148, 163, 184, 0.18)",
          color: "rgba(15, 23, 42, 0.94)",
          fontSize: 16,
          fontWeight: 700,
          background: "linear-gradient(180deg, rgba(255,255,255,0.68), rgba(255,255,255,0.28))",
        }}
      >
        <div style={{ minWidth: 0, flex: 1, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
          {peerTitle}
        </div>
        <div style={{ display: "flex", gap: 6, alignItems: "center" }}>
          <GlassButton
            type="button"
            onClick={() => void runSmartAction("summary")}
            disabled={smartBusy}
            style={{ padding: "5px 9px", fontSize: 12 }}
          >
            摘要
          </GlassButton>
          <GlassButton
            type="button"
            onClick={() => void runSmartAction("suggest")}
            disabled={smartBusy}
            style={{ padding: "5px 9px", fontSize: 12 }}
          >
            建议
          </GlassButton>
          <GlassButton
            type="button"
            onClick={() => void runSmartAction("translate")}
            disabled={smartBusy}
            style={{ padding: "5px 9px", fontSize: 12 }}
          >
            翻译
          </GlassButton>
        </div>
      </div>

      {(smartResult || smartError || smartBusy) ? (
        <GlassSurface
          style={{
            margin: "10px 14px 0",
            padding: "10px 12px",
            borderRadius: 12,
            background: smartError ? "rgba(232,65,65,0.08)" : undefined,
          }}
        >
          <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
            <div style={{ flex: 1, minWidth: 0, fontSize: 13, fontWeight: 700 }}>
              {smartTitle || "智能结果"}
            </div>
            <button
              type="button"
              aria-label="关闭智能结果"
              onClick={() => {
                setSmartResult("")
                setSmartError("")
                setSmartTitle("")
              }}
              style={{
                border: 0,
                background: "transparent",
                color: "var(--text-secondary)",
                cursor: "pointer",
                fontSize: 16,
                lineHeight: 1,
              }}
            >
              ×
            </button>
          </div>
          <div
            style={{
              marginTop: 6,
              maxHeight: 138,
              overflowY: "auto",
              whiteSpace: "pre-wrap",
              overflowWrap: "anywhere",
              fontSize: 13,
              lineHeight: 1.55,
              color: smartError ? "var(--color-badge)" : "var(--text-primary)",
            }}
          >
            {smartBusy ? "处理中..." : smartError || smartResult}
          </div>
        </GlassSurface>
      ) : null}

      {/* Message list */}
      <GlassScrollArea style={{ flex: 1, padding: "16px 0 18px" }}>
        {messages.map((msg, index) => {
          const previous = index > 0 ? messages[index - 1] : undefined
          const showAvatar = shouldShowMessageAvatar(previous, msg)
          const timeLabel = shouldShowTimeSeparator(previous, msg) ? formatConversationTime(msg.timestamp) : ""
          return (
            <div key={msg.clientMsgId}>
              {timeLabel ? (
                <div
                  style={{
                    display: "flex",
                    justifyContent: "center",
                    padding: index === 0 ? "10px 0 12px" : "18px 0 12px",
                  }}
                >
                  <span
                    style={{
                      minHeight: 20,
                      padding: "2px 10px",
                      borderRadius: 999,
                      background: "rgba(158, 174, 193, 0.36)",
                      color: "rgba(47, 65, 88, 0.72)",
                      fontSize: 11,
                      lineHeight: "16px",
                      fontWeight: 500,
                    }}
                  >
                    {timeLabel}
                  </span>
                </div>
              ) : null}
              <MessageBubble
                message={msg}
                showAvatar={showAvatar}
                stacked={!showAvatar}
              />
            </div>
          )
        })}
        <div ref={bottomRef} />
      </GlassScrollArea>

      {/* Composer */}
      <ComposerBar onSend={handleSend} />
    </div>
  )
}
