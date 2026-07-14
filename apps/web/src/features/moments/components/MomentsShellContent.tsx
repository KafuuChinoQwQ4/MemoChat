/** MomentsShellContent — moments feed */
import { useEffect, useRef, useState, type CSSProperties, type MouseEvent as ReactMouseEvent, type TransitionEvent as ReactTransitionEvent } from "react"
import { useQuery, useQueryClient } from "@tanstack/react-query"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { ENDPOINTS } from "@/core/config/endpoints"
import { useSessionStore } from "@/core/session/sessionStore"
import { displayNameWithoutInternalId } from "@/core/entities/displayIds"
import { useMediaUrl } from "@/shared/hooks/useMediaUrl"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { Spinner } from "@/shared/ui/primitives/Spinner"
import { formatMessageTime } from "@/shared/lib/time"

const TEXT_PREVIEW_MAX_LINES = 7
const TEXT_PREVIEW_LONG_THRESHOLD = 220
const MEDIA_PREVIEW_MAX_HEIGHT = 420
const MEDIA_BLOCK_GAP = 8

type MomentMediaType = "image" | "video"

interface MomentMedia {
  id: string
  type: MomentMediaType
  mediaKey: string
  previewKey: string
  width: number
  height: number
  durationMs: number
}

interface MomentItem {
  id: string
  authorUid: number
  authorName: string
  authorIcon?: string
  content: string
  media: MomentMedia[]
  location?: string
  createdAt: number
  likeCount: number
  commentCount: number
  hasLiked?: boolean
}

interface MomentContentItem {
  seq?: number
  media_type?: string
  media_key?: string
  thumb_key?: string
  content?: string
  width?: number
  height?: number
  duration_ms?: number
}

interface MomentDto {
  moment_id: number
  uid: number
  created_at: number
  like_count: number
  comment_count: number
  has_liked?: boolean
  liked?: boolean
  user_id?: string
  user_name?: string
  user_nick?: string
  user_icon?: string
  location?: string
  items?: MomentContentItem[]
}

interface FeedResponse {
  error: number
  moments?: MomentDto[]
}

interface PublishResponse {
  error?: number
  code?: number
  message?: string
  moment_id?: number
}

function normalizeMomentMediaType(type: string | undefined): MomentMediaType | null {
  const normalized = type?.trim().toLowerCase()
  if (normalized === "image" || normalized === "video") return normalized
  return null
}

function logicalLineCount(text: string): number {
  if (!text) return 0
  return text.split(/\r\n|\r|\n/).length
}

function hasTextPreviewOverflow(text: string): boolean {
  return text.length > TEXT_PREVIEW_LONG_THRESHOLD || logicalLineCount(text) > TEXT_PREVIEW_MAX_LINES
}

function mediaDisplayHeight(media: MomentMedia, full = false): number {
  if (media.type === "video") return full ? 236 : 188
  const sourceWidth = media.width > 0 ? media.width : 200
  const sourceHeight = media.height > 0 ? media.height : 200
  const aspect = sourceHeight / Math.max(1, sourceWidth)
  const baseWidth = full ? 420 : 220
  const minHeight = full ? 140 : 90
  const maxHeight = full ? 520 : 260
  return Math.min(maxHeight, Math.max(minHeight, Math.round(baseWidth * aspect)))
}

function mediaStackHeight(media: MomentMedia[], full = false): number {
  return media.reduce((total, item, index) => {
    const gap = index > 0 ? MEDIA_BLOCK_GAP : 0
    return total + gap + mediaDisplayHeight(item, full)
  }, 0)
}

function hasMediaPreviewOverflow(media: MomentMedia[]): boolean {
  return mediaStackHeight(media) > MEDIA_PREVIEW_MAX_HEIGHT + 1
}

function mapMoment(item: MomentDto): MomentItem {
  const textParts: string[] = []
  const media: MomentMedia[] = []

  ;(item.items ?? []).forEach((part, index) => {
    const type = normalizeMomentMediaType(part.media_type)
    const content = part.content?.trim() ?? ""
    const mediaKey = part.media_key?.trim() ?? ""
    const thumbKey = part.thumb_key?.trim() ?? ""
    const previewKey = thumbKey || mediaKey

    if (type && previewKey) {
      media.push({
        id: `${item.moment_id}-${part.seq ?? index}-${previewKey}`,
        type,
        mediaKey: mediaKey || previewKey,
        previewKey,
        width: Number(part.width ?? 0),
        height: Number(part.height ?? 0),
        durationMs: Number(part.duration_ms ?? 0),
      })
      return
    }

    if (content) {
      textParts.push(content)
    }
  })

  const textContent = textParts.join("\n")
  const authorName =
    displayNameWithoutInternalId(
      item.user_nick?.trim() || item.user_name?.trim(),
      item.user_id,
      item.uid,
      "未知用户",
    )
  const location = item.location?.trim() ?? ""
  return {
    id: String(item.moment_id),
    authorUid: item.uid,
    authorName,
    ...(item.user_icon ? { authorIcon: item.user_icon } : {}),
    content: textContent || location,
    media,
    ...(location ? { location } : {}),
    createdAt: Number(item.created_at || 0),
    likeCount: Math.max(0, Number(item.like_count ?? 0)),
    commentCount: Math.max(0, Number(item.comment_count ?? 0)),
    hasLiked: Boolean(item.has_liked ?? item.liked),
  }
}

function HeartIcon() {
  return (
    <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor"
      strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" aria-hidden>
      <path d="M12 19.2s-6.8-4-8.2-8.2C2.9 8.3 4.4 6.1 7 6.1c1.5 0 2.7.8 3.4 2 .7-1.2 1.9-2 3.4-2 2.6 0 4.1 2.2 3.2 4.9C18.8 15.2 12 19.2 12 19.2Z" />
    </svg>
  )
}

function CommentIcon() {
  return (
    <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor"
      strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" aria-hidden>
      <path d="M6.5 16.2h8.7c1.9 0 3.1-1.2 3.1-3V8.3c0-1.8-1.2-3-3.1-3H8.8c-1.9 0-3.1 1.2-3.1 3v4.8c0 1.2.5 2.1 1.5 2.7l-.4 2.1 2-1.7Z" />
      <path d="M8.6 9.4h6.5" />
      <path d="M8.6 12.2h4.2" />
    </svg>
  )
}

function previewTextStyle(text: string): CSSProperties {
  const style: CSSProperties = {
    fontSize: 14,
    lineHeight: 1.65,
    margin: 0,
    color: "var(--text-primary)",
    whiteSpace: "pre-wrap",
    overflowWrap: "anywhere",
  }
  if (!hasTextPreviewOverflow(text)) return style
  return {
    ...style,
    display: "-webkit-box",
    WebkitLineClamp: TEXT_PREVIEW_MAX_LINES,
    WebkitBoxOrient: "vertical",
    overflow: "hidden",
  }
}

interface SourceRect {
  top: number
  left: number
  width: number
  height: number
}

function readSourceRect(el: HTMLElement | null): SourceRect | null {
  if (!el) return null
  const rect = el.getBoundingClientRect()
  if (rect.width <= 0 || rect.height <= 0) return null
  return {
    top: rect.top,
    left: rect.left,
    width: rect.width,
    height: rect.height,
  }
}

function targetExpandRect(viewportWidth: number, viewportHeight: number): SourceRect {
  const maxWidth = Math.min(560, Math.max(320, viewportWidth - 48))
  const maxHeight = Math.min(720, Math.max(360, viewportHeight - 48))
  return {
    width: maxWidth,
    height: maxHeight,
    left: Math.round((viewportWidth - maxWidth) / 2),
    top: Math.round((viewportHeight - maxHeight) / 2),
  }
}

function formatDuration(durationMs: number): string {
  if (!durationMs || durationMs <= 0) return "点击打开视频"
  const totalSec = Math.floor(durationMs / 1000)
  const minutes = Math.floor(totalSec / 60)
  const seconds = totalSec % 60
  return `${minutes}:${seconds < 10 ? `0${seconds}` : seconds}`
}

function MomentMediaTile({
  media,
  full,
  onOpen,
}: {
  media: MomentMedia
  full?: boolean
  onOpen?: () => void
}) {
  const height = mediaDisplayHeight(media, full)
  const source = useMediaUrl(full ? media.mediaKey : media.previewKey)
  const previewSource = useMediaUrl(media.type === "video" ? media.previewKey : null)
  const interactive = Boolean(onOpen)
  const sharedStyle: CSSProperties = {
    width: "100%",
    height,
    borderRadius: 10,
    overflow: "hidden",
    border: "1px solid var(--divider)",
    background: "rgba(255,255,255,0.08)",
    cursor: interactive ? "pointer" : undefined,
  }

  if (media.type === "image") {
    return (
      <div
        role={interactive ? "button" : undefined}
        tabIndex={interactive ? 0 : undefined}
        onClick={onOpen}
        onKeyDown={(event) => {
          if (!interactive) return
          if (event.key === "Enter" || event.key === " ") {
            event.preventDefault()
            onOpen?.()
          }
        }}
        style={sharedStyle}
      >
        <img
          src={source}
          alt=""
          loading="lazy"
          style={{ width: "100%", height: "100%", objectFit: "cover", display: "block" }}
        />
      </div>
    )
  }

  const tile = (
    <div
      style={{
        ...sharedStyle,
        position: "relative",
        display: "grid",
        placeItems: "center",
        color: "#eaf2ff",
        background: previewSource
          ? `linear-gradient(rgba(18,25,34,0.42), rgba(18,25,34,0.72)), url("${previewSource}") center/cover`
          : "linear-gradient(135deg, #263447, #111821)",
      }}
    >
      <div style={{ textAlign: "center" }}>
        <div style={{ fontSize: full ? 40 : 34, lineHeight: 1 }}>▶</div>
        <div style={{ fontSize: 12, marginTop: 8 }}>{formatDuration(media.durationMs)}</div>
      </div>
    </div>
  )

  if (!full) {
    return (
      <div onClick={onOpen} style={{ cursor: interactive ? "pointer" : undefined }}>
        {tile}
      </div>
    )
  }

  return (
    <a href={source || undefined} target="_blank" rel="noreferrer" style={{ textDecoration: "none" }}>
      {tile}
    </a>
  )
}

function MomentMediaBlocks({
  media,
  full = false,
  onOpen,
}: {
  media: MomentMedia[]
  full?: boolean
  onOpen?: () => void
}) {
  if (media.length === 0) return null
  const overflow = !full && hasMediaPreviewOverflow(media)
  return (
    <div
      style={{
        position: "relative",
        display: "grid",
        gap: MEDIA_BLOCK_GAP,
        marginTop: 12,
        maxHeight: full ? undefined : MEDIA_PREVIEW_MAX_HEIGHT,
        overflow: full ? "visible" : "hidden",
      }}
    >
      {media.map((part) => (
        <MomentMediaTile
          key={part.id}
          media={part}
          full={full}
          {...(!full && onOpen ? { onOpen } : {})}
        />
      ))}
      {overflow ? (
        <button
          type="button"
          onClick={onOpen}
          style={{
            position: "absolute",
            left: 0,
            right: 0,
            bottom: 0,
            height: 72,
            border: 0,
            borderRadius: "0 0 10px 10px",
            padding: "0 12px 12px",
            display: "flex",
            alignItems: "flex-end",
            justifyContent: "flex-end",
            color: "#86b7ff",
            fontSize: 13,
            cursor: "pointer",
            background: "linear-gradient(rgba(20,24,32,0), rgba(20,24,32,0.78))",
          }}
        >
          查看全部
        </button>
      ) : null}
    </div>
  )
}

function MomentExpandOverlay({
  moment,
  sourceRect,
  onClose,
}: {
  moment: MomentItem
  sourceRect: SourceRect
  onClose: () => void
}) {
  const [expanded, setExpanded] = useState(false)
  const [closing, setClosing] = useState(false)
  const panelRef = useRef<HTMLDivElement | null>(null)
  const target = targetExpandRect(
    typeof window !== "undefined" ? window.innerWidth : 1200,
    typeof window !== "undefined" ? window.innerHeight : 800,
  )

  useEffect(() => {
    const frame = window.requestAnimationFrame(() => setExpanded(true))
    return () => window.cancelAnimationFrame(frame)
  }, [])

  useEffect(() => {
    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Escape") requestClose()
    }
    window.addEventListener("keydown", onKeyDown)
    return () => window.removeEventListener("keydown", onKeyDown)
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  function requestClose() {
    if (closing) return
    setClosing(true)
    setExpanded(false)
  }

  function handleTransitionEnd(event: ReactTransitionEvent<HTMLDivElement>) {
    if (event.target !== panelRef.current) return
    if (!closing) return
    if (event.propertyName !== "width" && event.propertyName !== "transform" && event.propertyName !== "top") {
      return
    }
    onClose()
  }

  const active = expanded && !closing
  const geometry = active ? target : sourceRect

  return (
    <div
      role="dialog"
      aria-modal="true"
      aria-label="动态全文"
      onClick={requestClose}
      style={{
        position: "fixed",
        inset: 0,
        zIndex: 90,
        background: active ? "rgba(2, 6, 12, 0.42)" : "rgba(2, 6, 12, 0)",
        backdropFilter: active ? "blur(8px)" : "blur(0px)",
        transition: "background 220ms ease, backdrop-filter 220ms ease",
      }}
    >
      <div
        ref={panelRef}
        onClick={(event) => event.stopPropagation()}
        onTransitionEnd={handleTransitionEnd}
        style={{
          position: "fixed",
          top: geometry.top,
          left: geometry.left,
          width: geometry.width,
          height: geometry.height,
          borderRadius: active ? 18 : 12,
          overflow: "hidden",
          boxShadow: active
            ? "0 24px 64px rgba(0,0,0,0.28), 0 8px 24px rgba(0,0,0,0.16)"
            : "0 1px 3px rgba(0,0,0,0.05), 0 4px 14px rgba(0,0,0,0.06)",
          transition:
            "top 240ms cubic-bezier(0.2, 0.85, 0.2, 1), left 240ms cubic-bezier(0.2, 0.85, 0.2, 1), width 240ms cubic-bezier(0.2, 0.85, 0.2, 1), height 240ms cubic-bezier(0.2, 0.85, 0.2, 1), border-radius 240ms ease, box-shadow 240ms ease",
          willChange: "top, left, width, height",
          zIndex: 91,
        }}
      >
        <GlassSurface
          elevated
          style={{
            width: "100%",
            height: "100%",
            borderRadius: "inherit",
            padding: active ? 18 : 15,
            display: "flex",
            flexDirection: "column",
            overflow: "hidden",
            transition: "padding 240ms ease",
          }}
        >
          <div style={{ display: "flex", alignItems: "center", gap: 12, marginBottom: 14, flexShrink: 0 }}>
            <Avatar src={moment.authorIcon} name={moment.authorName} size={active ? 40 : 36} />
            <div style={{ flex: 1, minWidth: 0 }}>
              <div style={{ fontSize: active ? 15 : 14, fontWeight: 700 }}>{moment.authorName}</div>
              <div style={{ fontSize: 12, color: "var(--text-disabled)", marginTop: 2 }}>
                {formatMessageTime(moment.createdAt)}
                {moment.location ? ` · ${moment.location}` : ""}
              </div>
            </div>
            <button
              type="button"
              onClick={requestClose}
              aria-label="关闭"
              style={{
                width: 34,
                height: 34,
                borderRadius: 10,
                border: "1px solid var(--divider)",
                color: "var(--text-secondary)",
                background: "rgba(255,255,255,0.08)",
                cursor: "pointer",
                fontSize: 18,
              }}
            >
              ×
            </button>
          </div>
          <div style={{ overflowY: "auto", paddingRight: 4, minHeight: 0, flex: 1 }}>
            {moment.content ? (
              <p
                style={{
                  fontSize: active ? 15 : 14,
                  lineHeight: 1.72,
                  whiteSpace: "pre-wrap",
                  overflowWrap: "anywhere",
                  color: "var(--text-primary)",
                  margin: 0,
                }}
              >
                {moment.content}
              </p>
            ) : null}
            <MomentMediaBlocks media={moment.media} full />
          </div>
        </GlassSurface>
      </div>
    </div>
  )
}

export function MomentsShellContent() {
  const uid = useSessionStore((s) => s.uid)
  const token = useSessionStore((s) => s.token)
  const queryClient = useQueryClient()
  const [expandedMoment, setExpandedMoment] = useState<{
    moment: MomentItem
    sourceRect: SourceRect
  } | null>(null)
  const cardRefs = useRef<Map<string, HTMLElement>>(new Map())
  const [commentDrafts, setCommentDrafts] = useState<Record<string, string>>({})
  const [commentOpenIds, setCommentOpenIds] = useState<Set<string>>(() => new Set())
  const [actionBusyIds, setActionBusyIds] = useState<Set<string>>(() => new Set())
  const [actionStatus, setActionStatus] = useState("")
  const [draftText, setDraftText] = useState("")
  const [draftLocation, setDraftLocation] = useState("")
  const [visibility, setVisibility] = useState(0)
  const [publishing, setPublishing] = useState(false)
  const [publishStatus, setPublishStatus] = useState("")
  const likedOptimisticRef = useRef<Map<string, boolean>>(new Map())
  const { data, isLoading, error } = useQuery({
    queryKey: ["moments", "feed", uid],
    enabled: uid !== null && token !== null,
    queryFn: async () => {
      if (uid === null || token === null) throw new Error("Missing moments auth")
      const response = await getGateway().http.post<FeedResponse>(ENDPOINTS.momentsList, {
        last_moment_id: 0,
        limit: 20,
      })
      if (response.error !== 0) throw new Error(`Moments list failed: ${response.error}`)
      return (response.moments ?? []).map(mapMoment)
    },
  })

  async function publishMoment() {
    const content = draftText.trim()
    if (!content) {
      setPublishStatus("先写一点内容再发布")
      return
    }
    if (uid === null || token === null) {
      setPublishStatus("登录状态无效，请重新登录")
      return
    }
    setPublishing(true)
    setPublishStatus("正在发布...")
    try {
      const response = await getGateway().http.post<PublishResponse>(ENDPOINTS.momentsPublish, {
        visibility,
        location: draftLocation.trim(),
        items: [{
          media_type: "text",
          content,
        }],
      })
      const errorCode = response.error ?? response.code ?? 0
      if (errorCode !== 0) {
        setPublishStatus(`发布失败，错误码 ${errorCode}`)
        return
      }
      setDraftText("")
      setDraftLocation("")
      setPublishStatus(response.moment_id ? `已发布动态 ${response.moment_id}` : "已发布")
      await queryClient.invalidateQueries({ queryKey: ["moments", "feed", uid] })
    } catch (err) {
      setPublishStatus(err instanceof Error ? err.message : "发布失败")
    } finally {
      setPublishing(false)
    }
  }


  function setBusy(momentId: string, busy: boolean) {
    setActionBusyIds((prev) => {
      const next = new Set(prev)
      if (busy) next.add(momentId)
      else next.delete(momentId)
      return next
    })
  }

  function patchMomentInCache(momentId: string, patch: Partial<MomentItem>) {
    queryClient.setQueryData<MomentItem[]>(["moments", "feed", uid], (prev) => {
      if (!prev) return prev
      return prev.map((item) => (item.id === momentId ? { ...item, ...patch } : item))
    })
  }

  function openMomentExpand(moment: MomentItem) {
    const sourceRect = readSourceRect(cardRefs.current.get(moment.id) ?? null)
      ?? targetExpandRect(window.innerWidth, window.innerHeight)
    setExpandedMoment({ moment, sourceRect })
  }

  function closeMomentExpand() {
    setExpandedMoment(null)
  }

  function toggleCommentComposer(momentId: string) {
    setCommentOpenIds((prev) => {
      const next = new Set(prev)
      if (next.has(momentId)) next.delete(momentId)
      else next.add(momentId)
      return next
    })
    setActionStatus("")
  }

  async function toggleLike(item: MomentItem) {
    if (uid === null || token === null) {
      setActionStatus("登录状态无效，请重新登录")
      return
    }
    if (actionBusyIds.has(item.id)) return

    const previousLiked = likedOptimisticRef.current.get(item.id) ?? Boolean(item.hasLiked)
    const previousCount = item.likeCount
    const nextLiked = !previousLiked
    const nextCount = Math.max(0, previousCount + (nextLiked ? 1 : -1))
    likedOptimisticRef.current.set(item.id, nextLiked)
    patchMomentInCache(item.id, { hasLiked: nextLiked, likeCount: nextCount })
    setBusy(item.id, true)
    setActionStatus(nextLiked ? "点赞中..." : "取消点赞中...")
    try {
      const response = await getGateway().http.post<{
        error?: number
        code?: number
        message?: string
        like_count?: number
        liked?: boolean
        has_liked?: boolean
      }>(
        ENDPOINTS.momentsLike,
        {
          moment_id: Number(item.id),
          like: nextLiked,
        },
      )
      const errorCode = response.error ?? response.code ?? 0
      if (errorCode !== 0) {
        likedOptimisticRef.current.set(item.id, previousLiked)
        patchMomentInCache(item.id, { hasLiked: previousLiked, likeCount: previousCount })
        setActionStatus(response.message?.trim() || `点赞失败，错误码 ${errorCode}`)
        return
      }
      const serverLiked = typeof response.has_liked === "boolean"
        ? response.has_liked
        : typeof response.liked === "boolean"
          ? response.liked
          : nextLiked
      const serverCount = typeof response.like_count === "number" ? Math.max(0, response.like_count) : nextCount
      likedOptimisticRef.current.set(item.id, serverLiked)
      patchMomentInCache(item.id, { hasLiked: serverLiked, likeCount: serverCount })
      setActionStatus(serverLiked ? "已点赞" : "已取消点赞")
    } catch (err) {
      likedOptimisticRef.current.set(item.id, previousLiked)
      patchMomentInCache(item.id, { hasLiked: previousLiked, likeCount: previousCount })
      setActionStatus(err instanceof Error ? err.message : "点赞失败")
    } finally {
      setBusy(item.id, false)
    }
  }

  async function submitComment(item: MomentItem) {
    if (uid === null || token === null) {
      setActionStatus("登录状态无效，请重新登录")
      return
    }
    const content = (commentDrafts[item.id] ?? "").trim()
    if (!content) {
      setActionStatus("先写一点评论")
      return
    }
    if (actionBusyIds.has(item.id)) return
    setBusy(item.id, true)
    setActionStatus("评论发送中...")
    try {
      const response = await getGateway().http.post<{ error?: number; code?: number; message?: string }>(
        ENDPOINTS.momentsComment,
        {
          moment_id: Number(item.id),
          content,
          reply_uid: 0,
        },
      )
      const errorCode = response.error ?? response.code ?? 0
      if (errorCode !== 0) {
        setActionStatus(response.message?.trim() || `评论失败，错误码 ${errorCode}`)
        return
      }
      patchMomentInCache(item.id, { commentCount: item.commentCount + 1 })
      setCommentDrafts((prev) => ({ ...prev, [item.id]: "" }))
      setActionStatus("评论已发送")
    } catch (err) {
      setActionStatus(err instanceof Error ? err.message : "评论失败")
    } finally {
      setBusy(item.id, false)
    }
  }

  if (isLoading) {
    return (
      <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }}>
        <Spinner size={30} />
      </div>
    )
  }

  if (error) {
    return (
      <div style={{
        height: "100%", display: "flex", alignItems: "center",
        justifyContent: "center", color: "var(--color-badge)", fontSize: 14,
      }}>
        加载失败，请刷新重试
      </div>
    )
  }

  const items = data ?? []

  return (
    <GlassScrollArea style={{ height: "100%", width: "100%", padding: "18px 14px 22px" }}>
      <div style={{ maxWidth: 1360, margin: "0 auto", width: "100%" }}>

        {/* Header */}
        <div style={{ marginBottom: 18, animation: "fade-up 200ms ease both" }}>
          <h2 style={{ fontSize: 18, fontWeight: 700 }}>朋友圈</h2>
          {actionStatus ? (
            <div style={{ marginTop: 6, fontSize: 12, color: "var(--text-secondary)" }}>{actionStatus}</div>
          ) : null}
        </div>

        <GlassSurface
          elevated
          style={{
            padding: 16,
            marginBottom: 14,
            borderRadius: 14,
            animation: "fade-up 210ms ease both",
          }}
        >
          <textarea
            value={draftText}
            onChange={(event) => setDraftText(event.target.value)}
            placeholder="发布朋友圈..."
            aria-label="朋友圈内容"
            maxLength={4096}
            style={{
              width: "100%",
              minHeight: 82,
              resize: "vertical",
              border: 0,
              borderRadius: 12,
              padding: "10px 12px",
              background: "rgba(255,255,255,0.10)",
              color: "var(--text-primary)",
              font: "inherit",
              fontSize: 14,
              lineHeight: 1.6,
              outline: "1px solid var(--divider)",
              boxSizing: "border-box",
            }}
          />
          <div
            style={{
              display: "grid",
              gridTemplateColumns: "minmax(0, 1fr) 132px auto",
              gap: 10,
              alignItems: "center",
              marginTop: 10,
            }}
          >
            <GlassTextField
              value={draftLocation}
              onChange={(event) => setDraftLocation(event.target.value)}
              placeholder="位置（可选）"
              aria-label="朋友圈位置"
            />
            <select
              value={visibility}
              onChange={(event) => setVisibility(Number(event.target.value))}
              aria-label="朋友圈可见范围"
              style={{
                height: 38,
                borderRadius: 10,
                border: "1px solid var(--divider)",
                background: "rgba(255,255,255,0.12)",
                color: "var(--text-primary)",
                padding: "0 10px",
                font: "inherit",
              }}
            >
              <option value={0}>公开</option>
              <option value={1}>好友可见</option>
            </select>
            <GlassButton
              type="button"
              variant="primary"
              onClick={() => void publishMoment()}
              disabled={publishing || !draftText.trim()}
            >
              {publishing ? "发布中" : "发布"}
            </GlassButton>
          </div>
          {publishStatus ? (
            <div style={{ marginTop: 8, fontSize: 12, color: "var(--text-secondary)" }}>
              {publishStatus}
            </div>
          ) : null}
        </GlassSurface>

        {items.length === 0 ? (
          <GlassSurface
            elevated
            style={{
              minHeight: 240,
              display: "grid",
              placeItems: "center",
              color: "var(--text-disabled)",
              fontSize: 14,
              animation: "fade-up 220ms ease both",
            }}
          >
            暂无动态
          </GlassSurface>
        ) : (
          <div
            style={{
              columnWidth: 236,
              columnGap: 12,
              width: "100%",
            }}
          >
            {items.map((item, idx) => (
            <div
              key={item.id}
              ref={(node) => {
                if (node) cardRefs.current.set(item.id, node)
                else cardRefs.current.delete(item.id)
              }}
              style={{
                display: "inline-block",
                width: "100%",
                breakInside: "avoid",
                margin: "0 0 12px",
              }}
            >
            <GlassSurface
              style={{
                width: "100%",
                padding: "15px 16px",
                borderRadius: 12,
                boxShadow: "0 1px 3px rgba(0,0,0,0.05), 0 4px 14px rgba(0,0,0,0.06)",
                transition: "transform 160ms ease, box-shadow 160ms ease",
                animation: `fade-up ${180 + idx * 30}ms cubic-bezier(0.4,0,0.2,1) both`,
              }}
              onMouseEnter={(e) => {
                const el = e.currentTarget as HTMLElement
                el.style.transform = "translateY(-2px)"
                el.style.boxShadow = "0 2px 6px rgba(0,0,0,0.07), 0 8px 24px rgba(0,0,0,0.09)"
              }}
              onMouseLeave={(e) => {
                const el = e.currentTarget as HTMLElement
                el.style.transform = ""
                el.style.boxShadow = "0 1px 3px rgba(0,0,0,0.05), 0 4px 14px rgba(0,0,0,0.06)"
              }}
            >
              {/* Author row */}
              <div style={{ display: "flex", gap: 10, marginBottom: 10, alignItems: "center" }}>
                <Avatar src={item.authorIcon} name={item.authorName} size={36} />
                <div style={{ flex: 1, minWidth: 0 }}>
                  <div style={{ fontWeight: 600, fontSize: 14, lineHeight: 1.3 }}>
                    {item.authorName}
                  </div>
                  <div style={{ fontSize: 11, color: "var(--text-disabled)", marginTop: 1 }}>
                    {formatMessageTime(item.createdAt)}
                  </div>
                </div>
              </div>

              {/* Content */}
              {item.content ? (
                <>
                  <p style={previewTextStyle(item.content)}>
                    {item.content}
                  </p>
                  {hasTextPreviewOverflow(item.content) ? (
                    <button
                      type="button"
                      onClick={(event: ReactMouseEvent<HTMLButtonElement>) => {
                        event.stopPropagation()
                        openMomentExpand(item)
                      }}
                      style={{
                        marginTop: 8,
                        padding: 0,
                        border: 0,
                        background: "transparent",
                        color: "#86b7ff",
                        fontSize: 13,
                        cursor: "pointer",
                      }}
                    >
                      查看全文
                    </button>
                  ) : null}
                </>
              ) : item.media.length === 0 ? (
                <p style={{ fontSize: 14, lineHeight: 1.65, margin: 0, color: "var(--text-disabled)" }}>
                  内容未保存，请重新发布
                </p>
              ) : null}

              <MomentMediaBlocks
                media={item.media}
                onOpen={() => openMomentExpand(item)}
              />

              {/* Metrics / actions */}
              <div style={{
                marginTop: 12,
                paddingTop: 10,
                borderTop: "1px solid var(--divider)",
                fontSize: 12,
                color: "var(--text-secondary)",
                display: "flex",
                gap: 10,
                flexWrap: "wrap",
                alignItems: "center",
              }}>
                <button
                  type="button"
                  onClick={(event) => {
                    event.stopPropagation()
                    void toggleLike(item)
                  }}
                  disabled={actionBusyIds.has(item.id)}
                  style={{
                    display: "inline-flex",
                    alignItems: "center",
                    gap: 4,
                    border: 0,
                    background: "transparent",
                    color: item.hasLiked
                      ? "var(--color-badge, #ff6b8a)"
                      : "var(--text-secondary)",
                    cursor: actionBusyIds.has(item.id) ? "not-allowed" : "pointer",
                    padding: "4px 6px",
                    borderRadius: 8,
                    font: "inherit",
                    fontSize: 12,
                  }}
                >
                  <HeartIcon /> {item.likeCount}
                </button>
                <button
                  type="button"
                  onClick={(event) => {
                    event.stopPropagation()
                    toggleCommentComposer(item.id)
                  }}
                  disabled={actionBusyIds.has(item.id)}
                  style={{
                    display: "inline-flex",
                    alignItems: "center",
                    gap: 4,
                    border: 0,
                    background: "transparent",
                    color: commentOpenIds.has(item.id) ? "#86b7ff" : "var(--text-secondary)",
                    cursor: actionBusyIds.has(item.id) ? "not-allowed" : "pointer",
                    padding: "4px 6px",
                    borderRadius: 8,
                    font: "inherit",
                    fontSize: 12,
                  }}
                >
                  <CommentIcon /> {item.commentCount}
                </button>
              </div>

              {commentOpenIds.has(item.id) ? (
                <div style={{ marginTop: 10, display: "grid", gap: 8 }}>
                  <textarea
                    value={commentDrafts[item.id] ?? ""}
                    onChange={(event) =>
                      setCommentDrafts((prev) => ({ ...prev, [item.id]: event.target.value }))
                    }
                    placeholder="写评论..."
                    aria-label={`评论动态 ${item.id}`}
                    maxLength={1000}
                    rows={2}
                    style={{
                      width: "100%",
                      resize: "vertical",
                      border: "1px solid var(--divider)",
                      borderRadius: 10,
                      padding: "8px 10px",
                      background: "rgba(255,255,255,0.10)",
                      color: "var(--text-primary)",
                      font: "inherit",
                      fontSize: 13,
                      lineHeight: 1.5,
                      boxSizing: "border-box",
                    }}
                  />
                  <div style={{ display: "flex", gap: 8 }}>
                    <GlassButton
                      type="button"
                      variant="primary"
                      disabled={actionBusyIds.has(item.id) || !(commentDrafts[item.id] ?? "").trim()}
                      onClick={() => void submitComment(item)}
                      style={{ flex: 1 }}
                    >
                      发送评论
                    </GlassButton>
                    <GlassButton
                      type="button"
                      disabled={actionBusyIds.has(item.id)}
                      onClick={() => toggleCommentComposer(item.id)}
                      style={{ flex: 1 }}
                    >
                      取消
                    </GlassButton>
                  </div>
                </div>
              ) : null}
            </GlassSurface>
            </div>
            ))}
          </div>
        )}
      </div>

      {expandedMoment ? (
        <MomentExpandOverlay
          moment={expandedMoment.moment}
          sourceRect={expandedMoment.sourceRect}
          onClose={closeMomentExpand}
        />
      ) : null}
    </GlassScrollArea>
  )
}
