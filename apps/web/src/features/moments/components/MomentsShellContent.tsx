/** MomentsShellContent — moments feed */
import { useQuery } from "@tanstack/react-query"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { ENDPOINTS } from "@/core/config/endpoints"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { Avatar } from "@/shared/ui/primitives/Avatar"
import { Spinner } from "@/shared/ui/primitives/Spinner"
import { formatMessageTime } from "@/shared/lib/time"

interface MomentItem {
  id: string
  authorUid: number
  authorName: string
  authorIcon?: string
  content: string
  createdAt: number
  likeCount: number
  commentCount: number
}

interface FeedResponse {
  error: number
  items?: MomentItem[]
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

export function MomentsShellContent() {
  const { data, isLoading, error } = useQuery({
    queryKey: ["moments", "feed"],
    queryFn: () => getGateway().http.get<FeedResponse>(`${ENDPOINTS.momentsList}?page=1`),
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

  const items = data?.items ?? []

  return (
    <GlassScrollArea style={{ height: "100%", padding: "16px 20px" }}>
      <div style={{ maxWidth: 600, margin: "0 auto" }}>

        {/* Header */}
        <div style={{ marginBottom: 20, animation: "fade-up 200ms ease both" }}>
          <h2 style={{ fontSize: 18, fontWeight: 700, letterSpacing: "-0.2px" }}>朋友圈</h2>
        </div>

        {items.length === 0 ? (
          <div style={{
            textAlign: "center", color: "var(--text-disabled)",
            fontSize: 14, padding: 60,
            animation: "fade-up 220ms ease both",
          }}>
            暂无动态
          </div>
        ) : (
          items.map((item, idx) => (
            <GlassSurface
              key={item.id}
              style={{
                marginBottom: 12,
                padding: "16px 18px",
                borderRadius: 14,
                boxShadow: "0 1px 3px rgba(0,0,0,0.05), 0 4px 14px rgba(0,0,0,0.06)",
                transition: "transform 160ms ease, box-shadow 160ms ease",
                animation: `fade-up ${180 + idx * 30}ms cubic-bezier(0.4,0,0.2,1) both`,
              }}
              /* hover lift via inline handlers to avoid extra CSS class */
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
                <Avatar name={item.authorName} size={36} />
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
              <p style={{ fontSize: 14, lineHeight: 1.65, margin: 0, color: "var(--text-primary)" }}>
                {item.content}
              </p>

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
          ))
        )}
      </div>
    </GlassScrollArea>
  )
}
