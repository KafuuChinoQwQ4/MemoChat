/** MomentsShellContent — moments feed (HTTP-backed with TanStack Query) */
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

export function MomentsShellContent() {
  const { data, isLoading, error } = useQuery({
    queryKey: ["moments", "feed"],
    queryFn: () =>
      getGateway().http.get<FeedResponse>(`${ENDPOINTS.momentsList}?page=1`),
  })

  if (isLoading) {
    return (
      <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }}>
        <Spinner size={32} />
      </div>
    )
  }

  if (error) {
    return (
      <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center", color: "var(--color-badge)", fontSize: 14 }}>
        加载失败，请刷新重试
      </div>
    )
  }

  const items = data?.items ?? []

  return (
    <GlassScrollArea style={{ height: "100%", padding: "12px 16px" }}>
      <div style={{ maxWidth: 600, margin: "0 auto" }}>
        <h2 style={{ fontSize: 17, fontWeight: 700, marginBottom: 16 }}>朋友圈</h2>
        {items.length === 0 ? (
          <div style={{ textAlign: "center", color: "var(--text-disabled)", fontSize: 14, padding: 48 }}>
            暂无动态
          </div>
        ) : (
          items.map((item) => (
            <GlassSurface key={item.id} style={{ marginBottom: 12, padding: 16, borderRadius: 12 }}>
              <div style={{ display: "flex", gap: 10, marginBottom: 8 }}>
                <Avatar name={item.authorName} size={36} />
                <div>
                  <div style={{ fontWeight: 600, fontSize: 14 }}>{item.authorName}</div>
                  <div style={{ fontSize: 11, color: "var(--text-disabled)" }}>{formatMessageTime(item.createdAt)}</div>
                </div>
              </div>
              <p style={{ fontSize: 14, lineHeight: 1.6, margin: 0 }}>{item.content}</p>
              <div style={{ marginTop: 10, fontSize: 12, color: "var(--text-secondary)", display: "flex", gap: 12 }}>
                <span>❤️ {item.likeCount}</span>
                <span>💬 {item.commentCount}</span>
              </div>
            </GlassSurface>
          ))
        )}
      </div>
    </GlassScrollArea>
  )
}
