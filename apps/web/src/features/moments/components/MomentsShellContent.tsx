/** MomentsShellContent — moments feed */
import { useState, type CSSProperties } from "react"
import { useQuery } from "@tanstack/react-query"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { ENDPOINTS } from "@/core/config/endpoints"
import { useSessionStore } from "@/core/session/sessionStore"
import { resolveMediaUrl } from "@/shared/media/mediaUrl"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
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
    item.user_nick?.trim() ||
    item.user_name?.trim() ||
    item.user_id?.trim() ||
    `用户 ${item.uid}`
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
  const source = resolveMediaUrl(full ? media.mediaKey : media.previewKey)
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
        background: media.previewKey
          ? `linear-gradient(rgba(18,25,34,0.42), rgba(18,25,34,0.72)), url("${resolveMediaUrl(media.previewKey)}") center/cover`
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
    <a href={resolveMediaUrl(media.mediaKey)} target="_blank" rel="noreferrer" style={{ textDecoration: "none" }}>
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

function MomentDetailOverlay({ moment, onClose }: { moment: MomentItem; onClose: () => void }) {
  return (
    <div
      role="dialog"
      aria-modal="true"
      onClick={onClose}
      style={{
        position: "fixed",
        inset: 0,
        zIndex: 80,
        padding: 24,
        display: "grid",
        placeItems: "center",
        background: "rgba(2, 6, 12, 0.58)",
        backdropFilter: "blur(14px)",
      }}
    >
      <GlassSurface
        elevated
        onClick={(event) => event.stopPropagation()}
        style={{
          width: "min(760px, calc(100vw - 48px))",
          maxHeight: "min(780px, calc(100vh - 48px))",
          padding: 18,
          borderRadius: 18,
          overflow: "hidden",
          display: "flex",
          flexDirection: "column",
        }}
      >
        <div style={{ display: "flex", alignItems: "center", gap: 12, marginBottom: 14 }}>
          <Avatar src={moment.authorIcon} name={moment.authorName} size={40} />
          <div style={{ flex: 1, minWidth: 0 }}>
            <div style={{ fontSize: 15, fontWeight: 700 }}>{moment.authorName}</div>
            <div style={{ fontSize: 12, color: "var(--text-disabled)", marginTop: 2 }}>
              {formatMessageTime(moment.createdAt)}
              {moment.location ? ` · ${moment.location}` : ""}
            </div>
          </div>
          <button
            type="button"
            onClick={onClose}
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
        <div style={{ overflowY: "auto", paddingRight: 4 }}>
          {moment.content ? (
            <p
              style={{
                fontSize: 15,
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
  )
}

export function MomentsShellContent() {
  const uid = useSessionStore((s) => s.uid)
  const token = useSessionStore((s) => s.token)
  const [selectedMoment, setSelectedMoment] = useState<MomentItem | null>(null)
  const { data, isLoading, error } = useQuery({
    queryKey: ["moments", "feed", uid],
    enabled: uid !== null && token !== null,
    queryFn: async () => {
      if (uid === null || token === null) throw new Error("Missing moments auth")
      const response = await getGateway().http.post<FeedResponse>(ENDPOINTS.momentsList, {
        uid,
        login_ticket: token,
        last_moment_id: 0,
        limit: 20,
      })
      if (response.error !== 0) throw new Error(`Moments list failed: ${response.error}`)
      return (response.moments ?? []).map(mapMoment)
    },
  })

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
        </div>

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
            <GlassSurface
              key={item.id}
              style={{
                display: "inline-block",
                width: "100%",
                breakInside: "avoid",
                margin: "0 0 12px",
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
                      onClick={() => setSelectedMoment(item)}
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

              <MomentMediaBlocks media={item.media} onOpen={() => setSelectedMoment(item)} />

              {/* Metrics */}
              <div style={{
                marginTop: 12,
                paddingTop: 10,
                borderTop: "1px solid var(--divider)",
                fontSize: 12,
                color: "var(--text-secondary)",
                display: "flex",
                gap: 16,
              }}>
                <span style={{ display: "inline-flex", alignItems: "center", gap: 4 }}>
                  <HeartIcon /> {item.likeCount}
                </span>
                <span style={{ display: "inline-flex", alignItems: "center", gap: 4 }}>
                  <CommentIcon /> {item.commentCount}
                </span>
              </div>
            </GlassSurface>
            ))}
          </div>
        )}
      </div>
      {selectedMoment ? (
        <MomentDetailOverlay moment={selectedMoment} onClose={() => setSelectedMoment(null)} />
      ) : null}
    </GlassScrollArea>
  )
}
