/** R18ShellContent — source switcher + search view for the R18 gateway domain. */
import { useEffect, useMemo, useState } from "react"
import { useQuery } from "@tanstack/react-query"
import { ENDPOINTS } from "@/core/config/endpoints"
import { useSessionStore } from "@/core/session/sessionStore"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { useMediaUrl } from "@/shared/hooks/useMediaUrl"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { Spinner } from "@/shared/ui/primitives/Spinner"

interface R18Source {
  id: string
  name?: string
  title?: string
  version?: string
  enabled?: boolean
  builtin?: boolean
  status?: string
  message?: string
}

interface R18ComicItem {
  source_id?: string
  comic_id?: string
  title?: string
  subtitle?: string
  description?: string
  cover?: string
  author?: string
  tags?: string[]
}

interface R18SourcesResponse {
  error: number
  message?: string
  data?: {
    sources?: R18Source[]
  }
}

interface R18SearchResponse {
  error: number
  message?: string
  data?: {
    source_id?: string
    items?: R18ComicItem[]
    max_page?: number
    error_message?: string
  }
}

function sourceTitle(source: R18Source): string {
  return source.title?.trim() || source.name?.trim() || source.id
}

function R18Cover({ cover, title }: { cover?: string; title?: string }) {
  const coverUrl = useMediaUrl(cover)
  if (!coverUrl) {
    return (
      <div style={{ aspectRatio: "3 / 4", display: "grid", placeItems: "center", color: "var(--text-disabled)", background: "var(--tint-hover)" }}>
        无封面
      </div>
    )
  }
  return (
    <img
      src={coverUrl}
      alt={title ?? "cover"}
      style={{ width: "100%", aspectRatio: "3 / 4", objectFit: "cover", background: "var(--tint-hover)" }}
    />
  )
}

export function R18ShellContent() {
  const uid = useSessionStore((s) => s.uid)
  const token = useSessionStore((s) => s.token)
  const [selectedSourceId, setSelectedSourceId] = useState("")
  const [keyword, setKeyword] = useState("")
  const [submittedKeyword, setSubmittedKeyword] = useState("")
  const [page, setPage] = useState(1)
  const [actionError, setActionError] = useState<string | null>(null)

  const authReady = uid !== null && token !== null

  const sourcesQuery = useQuery({
    queryKey: ["r18", "sources", uid],
    enabled: authReady,
    queryFn: async () => {
      if (uid === null || token === null) throw new Error("Missing R18 auth")
      const response = await getGateway().http.get<R18SourcesResponse>(ENDPOINTS.r18Sources)
      if (response.error !== 0) throw new Error(response.message || `R18 sources failed: ${response.error}`)
      return response.data?.sources ?? []
    },
  })

  const sources = useMemo(() => sourcesQuery.data ?? [], [sourcesQuery.data])
  const selectedSource = useMemo(
    () => sources.find((source) => source.id === selectedSourceId) ?? sources.find((source) => source.enabled) ?? sources[0],
    [sources, selectedSourceId],
  )

  useEffect(() => {
    if (!selectedSourceId && selectedSource?.id) {
      setSelectedSourceId(selectedSource.id)
    }
  }, [selectedSource?.id, selectedSourceId])

  const searchQuery = useQuery({
    queryKey: ["r18", "search", uid, selectedSource?.id, submittedKeyword, page],
    enabled: authReady && Boolean(selectedSource?.id),
    queryFn: async () => {
      if (uid === null || token === null || !selectedSource?.id) throw new Error("Missing R18 search input")
      const response = await getGateway().http.post<R18SearchResponse>(ENDPOINTS.r18Search, {
        source_id: selectedSource.id,
        keyword: submittedKeyword,
        page,
      })
      if (response.error !== 0) throw new Error(response.message || `R18 search failed: ${response.error}`)
      return response.data ?? { items: [] }
    },
  })

  async function toggleSource(source: R18Source) {
    if (uid === null || token === null) return
    const enable = !source.enabled
    setActionError(null)
    try {
      const response = await getGateway().http.post<{ error: number; message?: string }>(
        enable ? ENDPOINTS.r18SourceEnable : ENDPOINTS.r18SourceDisable,
        { source_id: source.id },
      )
      if (response.error !== 0) throw new Error(response.message || "源切换失败")
      await sourcesQuery.refetch()
    } catch (err) {
      setActionError(err instanceof Error ? err.message : "源切换失败")
    }
  }

  function submitSearch() {
    setSubmittedKeyword(keyword.trim())
    setPage(1)
  }

  if (!authReady) {
    return (
      <div style={{ height: "100%", width: "100%", display: "grid", placeItems: "center", color: "var(--text-disabled)" }}>
        登录状态无效
      </div>
    )
  }

  return (
    <div style={{ height: "100%", width: "100%", minWidth: 0, display: "grid", gridTemplateColumns: "280px minmax(0, 1fr)", overflow: "hidden" }}>
      <aside style={{ borderRight: "1px solid var(--divider)", display: "flex", flexDirection: "column", minHeight: 0 }}>
        <div style={{ padding: "14px 14px 10px" }}>
          <div style={{ fontSize: 16, fontWeight: 700 }}>R18</div>
          <div style={{ marginTop: 4, fontSize: 12, color: "var(--text-secondary)" }}>内容源</div>
        </div>
        <GlassScrollArea style={{ flex: 1, minHeight: 0, padding: "0 8px 12px" }}>
          {sourcesQuery.isLoading ? (
            <div style={{ display: "grid", placeItems: "center", padding: 28 }}>
              <Spinner size={24} />
            </div>
          ) : sources.map((source) => {
            const active = source.id === selectedSource?.id
            return (
              <button
                key={source.id}
                onClick={() => {
                  setSelectedSourceId(source.id)
                  setPage(1)
                }}
                style={{
                  width: "100%",
                  border: "none",
                  borderRadius: 10,
                  padding: "10px 12px",
                  marginBottom: 6,
                  textAlign: "left",
                  cursor: "pointer",
                  background: active ? "var(--tint-selected)" : "transparent",
                  color: "var(--text-primary)",
                }}
              >
                <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", gap: 8 }}>
                  <span style={{ minWidth: 0, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", fontWeight: 600 }}>
                    {sourceTitle(source)}
                  </span>
                  <span style={{
                    flexShrink: 0,
                    width: 8,
                    height: 8,
                    borderRadius: "50%",
                    background: source.enabled ? "var(--color-brand-green)" : "var(--text-disabled)",
                  }} />
                </div>
                <div style={{ marginTop: 4, fontSize: 12, color: "var(--text-secondary)" }}>
                  {source.version || source.status || source.id}
                </div>
              </button>
            )
          })}
        </GlassScrollArea>
      </aside>

      <section style={{ minWidth: 0, display: "flex", flexDirection: "column", overflow: "hidden" }}>
        <div style={{ padding: "14px 18px", borderBottom: "1px solid var(--divider)", display: "flex", gap: 10, alignItems: "center" }}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <GlassTextField
              value={keyword}
              onChange={(e) => setKeyword(e.target.value)}
              placeholder="搜索关键词"
              onKeyDown={(e) => {
                if (e.key === "Enter") {
                  e.preventDefault()
                  submitSearch()
                }
              }}
            />
          </div>
          <GlassButton onClick={submitSearch} variant="primary">搜索</GlassButton>
          {selectedSource && (
            <GlassButton onClick={() => { void toggleSource(selectedSource) }}>
              {selectedSource.enabled ? "停用源" : "启用源"}
            </GlassButton>
          )}
        </div>

        {(sourcesQuery.error || searchQuery.error || actionError) && (
          <div style={{ margin: "12px 18px 0", color: "var(--color-badge)", fontSize: 13 }}>
            {actionError ||
              (sourcesQuery.error instanceof Error ? sourcesQuery.error.message : null) ||
              (searchQuery.error instanceof Error ? searchQuery.error.message : "加载失败")}
          </div>
        )}

        <GlassScrollArea style={{ flex: 1, minHeight: 0, padding: 18 }}>
          {searchQuery.isLoading ? (
            <div style={{ display: "grid", placeItems: "center", height: "100%" }}>
              <Spinner size={28} />
            </div>
          ) : (
            <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(190px, 1fr))", gap: 14 }}>
              {(searchQuery.data?.items ?? []).map((item) => {
                return (
                  <GlassSurface key={`${item.source_id}-${item.comic_id}`} elevated style={{ overflow: "hidden" }}>
                    <R18Cover
                      {...(item.cover === undefined ? {} : { cover: item.cover })}
                      {...(item.title === undefined ? {} : { title: item.title })}
                    />
                    <div style={{ padding: 12 }}>
                      <div style={{ fontWeight: 700, fontSize: 14, lineHeight: 1.35, overflow: "hidden", display: "-webkit-box", WebkitLineClamp: 2, WebkitBoxOrient: "vertical" }}>
                        {item.title || item.comic_id || "未命名"}
                      </div>
                      {(item.subtitle || item.author) && (
                        <div style={{ marginTop: 6, fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                          {item.subtitle || item.author}
                        </div>
                      )}
                    </div>
                  </GlassSurface>
                )
              })}
              {(searchQuery.data?.items ?? []).length === 0 && !searchQuery.isLoading && (
                <GlassSurface style={{ minHeight: 220, display: "grid", placeItems: "center", color: "var(--text-disabled)", gridColumn: "1 / -1" }}>
                  暂无结果
                </GlassSurface>
              )}
            </div>
          )}
        </GlassScrollArea>
      </section>
    </div>
  )
}
