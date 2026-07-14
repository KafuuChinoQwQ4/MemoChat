/**
 * ConversationPane — full conversation view: message history + composer.
 * Handles message sending via chatApi (optimistic) and reads from entityStore.
 */
import { useCallback, useEffect, useLayoutEffect, useRef, useMemo, useState } from "react"
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
import { GroupManagementPanel } from "@/features/group/components/GroupManagementPanel"
import { useGroupManagement } from "@/features/group/hooks/useGroupManagement"
import { useGroupMembers } from "@/features/group/hooks/useGroupMembers"

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
  const scrollAreaRef = useRef<HTMLDivElement>(null)
  const messageRefs = useRef(new Map<string, HTMLDivElement>())
  const stickToBottomRef = useRef(true)
  const openingRef = useRef(true)
  const lastPeerIdRef = useRef(peerId)
  const lastMessageCountRef = useRef(0)
  const unreadJumpRafRef = useRef<number | null>(null)
  const suppressNextStickRef = useRef(false)
  const [smartBusy, setSmartBusy] = useState(false)
  const [smartTitle, setSmartTitle] = useState("")
  const [smartResult, setSmartResult] = useState("")
  const [smartError, setSmartError] = useState("")
  const [unreadBannerCount, setUnreadBannerCount] = useState(0)
  const [firstUnreadClientMsgId, setFirstUnreadClientMsgId] = useState<string | null>(null)
  const [jumpingToUnread, setJumpingToUnread] = useState(false)
  const [readyToShow, setReadyToShow] = useState(false)
  const [groupManageOpen, setGroupManageOpen] = useState(false)
  const friendsList = useMemo(() => Array.from(friendsMap.values()), [friendsMap])
  const selectedGroup = selectedIsGroup ? groupsMap.get(peerId) : undefined
  const groupManagement = useGroupManagement(selectedIsGroup ? peerId : null)
  const groupMembers = useGroupMembers(selectedGroup, friendsList, messages)
  const pendingGroupApplies = useEntityStore((s) => s.pendingGroupApplies)

  useEffect(() => {
    setGroupManageOpen(false)
  }, [peerId, selectedIsGroup])
  const peerTitle = useMemo(() => {
    const dialogTitle = dialogsMap.get(peerId)?.title?.trim()
    if (selectedIsGroup) {
      const group = groupsMap.get(peerId)
      return groupTitleWithoutInternalId(group?.name || dialogTitle, groupPublicIdText(group?.groupCode), peerId)
    }
    const friend = friendsMap.get(peerId)
    return displayNameWithoutInternalId(friend?.name || dialogTitle, friend?.userId, peerId, "未知用户")
  }, [dialogsMap, friendsMap, groupsMap, peerId, selectedIsGroup])

  const unreadCount = dialogsMap.get(peerId)?.unreadCount ?? 0

  const cancelUnreadJump = useCallback(() => {
    if (unreadJumpRafRef.current !== null) {
      cancelAnimationFrame(unreadJumpRafRef.current)
      unreadJumpRafRef.current = null
    }
    setJumpingToUnread(false)
  }, [])

  const isNearBottom = useCallback((el: HTMLDivElement, threshold = 80) => {
    return el.scrollHeight - el.scrollTop - el.clientHeight <= threshold
  }, [])

  const pinToBottomInstant = useCallback(() => {
    const area = scrollAreaRef.current
    if (!area) {
      bottomRef.current?.scrollIntoView({ behavior: "instant" as ScrollBehavior, block: "end" })
      stickToBottomRef.current = true
      return
    }
    // Force non-animated positioning. Never rely on CSS smooth defaults.
    area.style.scrollBehavior = "auto"
    const top = area.scrollHeight
    try {
      area.scrollTo({ top, behavior: "instant" as ScrollBehavior })
    } catch {
      area.scrollTop = top
    }
    // Some engines ignore scrollTo(instant); hard-assign as final authority.
    if (Math.abs(area.scrollTop + area.clientHeight - area.scrollHeight) > 2) {
      area.scrollTop = area.scrollHeight
    }
    stickToBottomRef.current = true
  }, [])

  const scrollToBottom = useCallback((behavior: ScrollBehavior = "auto") => {
    if (behavior === "smooth") {
      const area = scrollAreaRef.current
      if (!area) {
        bottomRef.current?.scrollIntoView({ behavior: "smooth", block: "end" })
      } else {
        area.scrollTo({ top: area.scrollHeight, behavior: "smooth" })
      }
      stickToBottomRef.current = true
      return
    }
    pinToBottomInstant()
  }, [pinToBottomInstant])

  const markConversationRead = useCallback(() => {
    const dialog = useEntityStore.getState().dialogs.get(peerId)
    if (!dialog || dialog.unreadCount <= 0) return

    // Clear sidebar badge only. Keep the in-conversation unread jump banner
    // until the user clicks it or leaves the chat.
    useEntityStore.getState().clearDialogUnread(peerId)

    if (selectedIsGroup || myUid <= 0) return
    const msgs = useEntityStore.getState().messages.get(peerId) ?? []
    for (let i = msgs.length - 1; i >= 0; i -= 1) {
      const candidate = msgs[i]
      if (!candidate || candidate.fromUid === myUid) continue
      if (typeof candidate.serverMsgId === "number" && candidate.serverMsgId > 0) {
        api.sendReadAck(peerId, candidate.serverMsgId)
        break
      }
    }
  }, [api, myUid, peerId, selectedIsGroup])

  // Reset per-conversation scroll/unread state when switching chats.
  // useLayoutEffect: pin BEFORE paint so the list never flashes from the top.
  useLayoutEffect(() => {
    cancelUnreadJump()
    lastPeerIdRef.current = peerId
    lastMessageCountRef.current = 0
    stickToBottomRef.current = true
    openingRef.current = true
    suppressNextStickRef.current = false
    setReadyToShow(false)
    setUnreadBannerCount(0)
    setFirstUnreadClientMsgId(null)

    // Capture unread snapshot before clearing badge, so the banner can jump later.
    const dialogUnread = useEntityStore.getState().dialogs.get(peerId)?.unreadCount ?? 0
    if (dialogUnread > 0) {
      setUnreadBannerCount(dialogUnread)
    }

    pinToBottomInstant()

    // Keep pinning across late layout/history paints without animating.
    let cancelled = false
    let revealTimer: number | null = null
    const snap = () => {
      if (cancelled) return
      if (openingRef.current || stickToBottomRef.current) pinToBottomInstant()
    }
    const raf1 = requestAnimationFrame(() => {
      snap()
      requestAnimationFrame(() => {
        snap()
        if (!cancelled) {
          // Reveal only after the first double-rAF pin so users never see top→bottom motion.
          setReadyToShow(true)
        }
      })
    })
    const t1 = window.setTimeout(snap, 0)
    const t2 = window.setTimeout(snap, 48)
    const t3 = window.setTimeout(snap, 120)
    const t4 = window.setTimeout(() => {
      snap()
      openingRef.current = false
    }, 280)
    // Failsafe reveal if messages never arrive.
    revealTimer = window.setTimeout(() => {
      if (!cancelled) setReadyToShow(true)
    }, 80)

    return () => {
      cancelled = true
      cancelAnimationFrame(raf1)
      window.clearTimeout(t1)
      window.clearTimeout(t2)
      window.clearTimeout(t3)
      window.clearTimeout(t4)
      if (revealTimer !== null) window.clearTimeout(revealTimer)
    }
  }, [cancelUnreadJump, peerId, pinToBottomInstant])

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

  // Pin to latest before paint on hydrate/history growth. Never animate open.
  useLayoutEffect(() => {
    if (lastPeerIdRef.current !== peerId) {
      lastPeerIdRef.current = peerId
      lastMessageCountRef.current = messages.length
      return
    }

    const prevCount = lastMessageCountRef.current
    lastMessageCountRef.current = messages.length
    if (messages.length === 0) return

    if (suppressNextStickRef.current) {
      suppressNextStickRef.current = false
      return
    }

    // Opening / first content / stick-to-bottom: hard pin, no smooth scroll.
    if (openingRef.current || prevCount === 0 || stickToBottomRef.current) {
      pinToBottomInstant()
      if (!readyToShow) setReadyToShow(true)
    }
  }, [messages, peerId, pinToBottomInstant, readyToShow])

  // Resolve first unread marker after content is available (state updates OK after paint).
  useEffect(() => {
    if (unreadBannerCount <= 0 || firstUnreadClientMsgId || messages.length === 0) return
    const start = Math.max(0, messages.length - unreadBannerCount)
    let found: string | null = null
    for (let i = start; i < messages.length; i += 1) {
      const msg = messages[i]
      if (msg && msg.fromUid !== myUid && !msg.isRevoked) {
        found = msg.clientMsgId
        break
      }
    }
    if (!found) {
      found = messages[start]?.clientMsgId ?? null
    }
    if (found) setFirstUnreadClientMsgId(found)
  }, [firstUnreadClientMsgId, messages, myUid, unreadBannerCount])

  // Keep pinned while opening if content height grows (images/layout).
  useLayoutEffect(() => {
    const area = scrollAreaRef.current
    if (!area) return

    const keepPinned = () => {
      if (openingRef.current || stickToBottomRef.current) {
        pinToBottomInstant()
      }
    }

    const ro = typeof ResizeObserver !== "undefined" ? new ResizeObserver(keepPinned) : null
    ro?.observe(area)
    const child = area.firstElementChild
    if (child instanceof HTMLElement) ro?.observe(child)

    return () => {
      ro?.disconnect()
    }
  }, [peerId, pinToBottomInstant])

  // Clear server/local unread after the conversation is opened and has content.
  useEffect(() => {
    if (messages.length === 0 || unreadCount <= 0) return
    const timer = window.setTimeout(() => {
      markConversationRead()
    }, 120)
    return () => window.clearTimeout(timer)
  }, [markConversationRead, messages.length, peerId, unreadCount])

  // User scroll cancels auto jump-to-unread and updates stick-to-bottom.
  useEffect(() => {
    const area = scrollAreaRef.current
    if (!area) return

    const onScroll = () => {
      stickToBottomRef.current = isNearBottom(area)
      if (jumpingToUnread) {
        cancelUnreadJump()
      }
    }
    const onWheel = () => {
      if (jumpingToUnread) cancelUnreadJump()
      // User actively scrolling during open: stop forced pin.
      if (openingRef.current) openingRef.current = false
    }
    const onTouchMove = () => {
      if (jumpingToUnread) cancelUnreadJump()
      if (openingRef.current) openingRef.current = false
    }
    const onPointerDown = () => {
      if (jumpingToUnread) cancelUnreadJump()
    }

    area.addEventListener("scroll", onScroll, { passive: true })
    area.addEventListener("wheel", onWheel, { passive: true })
    area.addEventListener("touchmove", onTouchMove, { passive: true })
    area.addEventListener("pointerdown", onPointerDown, { passive: true })
    return () => {
      area.removeEventListener("scroll", onScroll)
      area.removeEventListener("wheel", onWheel)
      area.removeEventListener("touchmove", onTouchMove)
      area.removeEventListener("pointerdown", onPointerDown)
    }
  }, [cancelUnreadJump, isNearBottom, jumpingToUnread, peerId])

  useEffect(() => () => cancelUnreadJump(), [cancelUnreadJump])

  const jumpToFirstUnread = useCallback(() => {
    const area = scrollAreaRef.current
    const targetId = firstUnreadClientMsgId
    if (!area || !targetId) {
      setUnreadBannerCount(0)
      setFirstUnreadClientMsgId(null)
      return
    }

    const target = messageRefs.current.get(targetId)
    if (!target) {
      setUnreadBannerCount(0)
      setFirstUnreadClientMsgId(null)
      return
    }

    cancelUnreadJump()
    openingRef.current = false
    stickToBottomRef.current = false
    suppressNextStickRef.current = true
    setJumpingToUnread(true)

    const destination = Math.max(0, target.offsetTop - 24)
    const start = area.scrollTop
    const distance = destination - start
    if (Math.abs(distance) < 2) {
      area.style.scrollBehavior = "auto"
      area.scrollTop = destination
      setJumpingToUnread(false)
      setUnreadBannerCount(0)
      setFirstUnreadClientMsgId(null)
      return
    }

    // Smooth programmatic scroll that user input can interrupt at any time.
    const durationMs = Math.min(1400, Math.max(280, Math.abs(distance) * 0.45))
    const startedAt = performance.now()

    const step = (now: number) => {
      if (unreadJumpRafRef.current === null) return
      const t = Math.min(1, (now - startedAt) / durationMs)
      // easeInOutCubic
      const eased = t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2
      area.scrollTop = start + distance * eased
      if (t < 1) {
        unreadJumpRafRef.current = requestAnimationFrame(step)
        return
      }
      unreadJumpRafRef.current = null
      setJumpingToUnread(false)
      setUnreadBannerCount(0)
      setFirstUnreadClientMsgId(null)
    }

    unreadJumpRafRef.current = requestAnimationFrame(step)
  }, [cancelUnreadJump, firstUnreadClientMsgId])

  function handleSend(text: string) {
    if (myUid <= 0) return
    stickToBottomRef.current = true
    cancelUnreadJump()
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
        position: "relative",
        height: "100%",
        display: "flex",
        flexDirection: "column",
        background: "var(--conversation-bg)",
      }}
    >
      {/* Topbar — frosted glass nickname header */}
      <div
        style={{
          position: "relative",
          minHeight: 52,
          padding: "0 14px",
          display: "flex",
          alignItems: "center",
          gap: 10,
          borderBottom: "1px solid var(--conversation-topbar-border)",
          color: "var(--conversation-topbar-color)",
          fontSize: 15,
          fontWeight: 600,
          background: "var(--conversation-topbar-bg)",
          backdropFilter: "blur(var(--conversation-topbar-blur, 22px)) saturate(1.65)",
          WebkitBackdropFilter: "blur(var(--conversation-topbar-blur, 22px)) saturate(1.65)",
          boxShadow: "var(--conversation-topbar-shadow, none)",
          // Keep messages scrolling underneath the frosted strip.
          zIndex: 2,
          flexShrink: 0,
          isolation: "isolate",
          overflow: "hidden",
        }}
      >
        {/* Top-edge sheen for liquid-glass feel */}
        <div
          aria-hidden
          style={{
            position: "absolute",
            inset: 0,
            pointerEvents: "none",
            background:
              "linear-gradient(to bottom, rgba(255,255,255,0.34) 0%, rgba(255,255,255,0.08) 42%, transparent 70%)",
            opacity: 0.9,
            zIndex: 0,
          }}
        />
        <div style={{ position: "relative", zIndex: 1, minWidth: 0, flex: 1, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
          {peerTitle}
        </div>
        <div style={{ position: "relative", zIndex: 1, display: "flex", gap: 6, alignItems: "center" }}>
          {selectedIsGroup ? (
            <GlassButton
              type="button"
              onClick={() => setGroupManageOpen(true)}
              style={{ padding: "5px 9px", fontSize: 12 }}
            >
              群管理
            </GlassButton>
          ) : null}
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
      <div style={{ position: "relative", flex: 1, minHeight: 0, display: "flex", flexDirection: "column" }}>
        {unreadBannerCount > 0 && firstUnreadClientMsgId ? (
          <button
            type="button"
            onClick={jumpToFirstUnread}
            style={{
              position: "absolute",
              top: 10,
              left: "50%",
              transform: "translateX(-50%)",
              zIndex: 3,
              border: "1px solid var(--conversation-topbar-border)",
              borderRadius: 999,
              padding: "6px 12px",
              background: "var(--conversation-topbar-bg)",
              color: "var(--conversation-topbar-color)",
              backdropFilter: "blur(var(--conversation-topbar-blur, 18px)) saturate(1.5)",
              WebkitBackdropFilter: "blur(var(--conversation-topbar-blur, 18px)) saturate(1.5)",
              boxShadow: "var(--conversation-topbar-shadow, 0 6px 18px rgba(0,0,0,0.12))",
              cursor: "pointer",
              fontSize: 12,
              fontWeight: 600,
              whiteSpace: "nowrap",
            }}
          >
            {jumpingToUnread
              ? "正在跳转到未读…"
              : unreadBannerCount === 1
                ? "1 条未读 · 点击跳转"
                : `${unreadBannerCount} 条未读 · 点击跳转`}
          </button>
        ) : null}

        <GlassScrollArea
          ref={scrollAreaRef}
          style={{
            flex: 1,
            minHeight: 0,
            padding: "16px 0 18px",
            // Hide until first instant pin to latest completes — prevents top→bottom flash.
            opacity: readyToShow ? 1 : 0,
            overflowAnchor: "none",
          }}
        >
          {messages.map((msg, index) => {
            const previous = index > 0 ? messages[index - 1] : undefined
            const showAvatar = shouldShowMessageAvatar(previous, msg)
            const timeLabel = shouldShowTimeSeparator(previous, msg) ? formatConversationTime(msg.timestamp) : ""
            const isFirstUnread = firstUnreadClientMsgId === msg.clientMsgId
            return (
              <div
                key={msg.clientMsgId}
                ref={(node) => {
                  if (node) messageRefs.current.set(msg.clientMsgId, node)
                  else messageRefs.current.delete(msg.clientMsgId)
                }}
              >
                {isFirstUnread ? (
                  <div
                    style={{
                      display: "flex",
                      alignItems: "center",
                      gap: 10,
                      padding: "10px 16px 8px",
                    }}
                  >
                    <div style={{ flex: 1, height: 1, background: "rgba(139, 92, 246, 0.35)" }} />
                    <span
                      style={{
                        fontSize: 11,
                        fontWeight: 600,
                        color: "var(--color-brand-green)",
                        letterSpacing: "0.02em",
                        whiteSpace: "nowrap",
                      }}
                    >
                      以下为未读消息
                    </span>
                    <div style={{ flex: 1, height: 1, background: "rgba(139, 92, 246, 0.35)" }} />
                  </div>
                ) : null}
                {timeLabel ? (
                  <div
                    style={{
                      display: "flex",
                      justifyContent: "center",
                      padding: index === 0 && !isFirstUnread ? "10px 0 12px" : "18px 0 12px",
                    }}
                  >
                    <span
                      style={{
                        minHeight: 20,
                        padding: "2px 10px",
                        borderRadius: 999,
                        background: "var(--time-sep-bg)",
                        color: "var(--time-sep-color)",
                        fontSize: 11,
                        lineHeight: "16px",
                        fontWeight: 500,
                        letterSpacing: "0.01em",
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
      </div>

      {/* Composer */}
      <ComposerBar onSend={handleSend} />

      {selectedIsGroup && selectedGroup ? (
        <GroupManagementPanel
          group={selectedGroup}
          friends={friendsList}
          members={groupMembers}
          pendingApplies={pendingGroupApplies}
          open={groupManageOpen}
          statusText={groupManagement.statusText}
          statusError={groupManagement.statusError}
          busy={groupManagement.busy}
          onClose={() => {
            setGroupManageOpen(false)
            groupManagement.clearStatus()
          }}
          actions={groupManagement.actions}
        />
      ) : null}
    </div>
  )
}
