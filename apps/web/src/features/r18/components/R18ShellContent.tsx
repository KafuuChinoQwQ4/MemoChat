/** R18ShellContent — source switcher + search + chapter reader */
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

// ─── Types ────────────────────────────────────────────────────────────────

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

interface R18Chapter {
  chapter_id: string
  title?: string
  index?: number
}

interface R18SourcesResponse {
  error: number
  message?: string
  data?: { sources?: R18Source[] }
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

interface R18ChaptersResponse {
  error: number
  message?: string
  data?: { chapters?: R18Chapter[] }
}

interface R18ChapterImagesResponse {
  error: number
  message?: string
  data?: { images?: string[] }
}

// ─── Helpers ──────────────────────────────────────────────────────────────

function sourceTitle(source: R18Source): string {
  return source.title?.trim() || source.name?.trim() || source.id
}

// ─── Sub-components ───────────────────────────────────────────────────────

function R18Cover({ cover, title }: { cover?: string; title?: string }) {
  const coverUrl = useMediaUrl(cover)
  return coverUrl ? (
    <img
      src={coverUrl}
      alt={title ?? "cover"}
      loading="lazy"
      style={{ width: "100%", aspectRatio: "3 / 4", objectFit: "cover", display: "block", background: "var(--tint-hover)" }}
    />
  ) : (
    <div style={{ aspectRatio: "3 / 4", display: "grid", placeItems: "center", color: "var(--text-disabled)", fontSize: 12, background: "var(--tint-hover)" }}>
      无封面
    </div>
  )
}

function CloseButton({ onClose }: { onClose: () => void }) {
  return (
    <button
      type="button"
      aria-label="关闭"
      onClick={onClose}
      style={{
        width: 34, height: 34, flexShrink: 0,
        borderRadius: 10, border: "1px solid var(--divider)",
        background: "rgba(255,255,255,0.08)", color: "var(--text-secondary)",
        cursor: "pointer", fontSize: 18, lineHeight: 1,
        display: "grid", placeItems: "center",
      }}
    >
      ×
    </button>
  )
}

// ─── Chapter list overlay ─────────────────────────────────────────────────

function ChapterListOverlay({
  comic,
  sourceId,
  onClose,
  onSelectChapter,
}: {
  comic: R18ComicItem
  sourceId: string
  onClose: () => void
  onSelectChapter: (chapterId: string, title: string) => void
}) {
  const chaptersQuery = useQuery({
    queryKey: ["r18", "chapters", sourceId, comic.comic_id],
    enabled: Boolean(sourceId && comic.comic_id),
    queryFn: async () => {
      const res = await getGateway().http.post<R18ChaptersResponse>(ENDPOINTS.r18ComicChapters, {
        source_id: sourceId,
        comic_id: comic.comic_id,
      })
      if (res.error !== 0) throw new Error(res.message || `获取章节失败: ${res.error}`)
      return res.data?.chapters ?? []
    },
  })

  return (
    <div
      role="dialog"
      aria-modal="true"
      onClick={onClose}
      style={{
        position: "fixed", inset: 0, zIndex: 90,
        display: "grid", placeItems: "center", padding: 24,
        background: "rgba(2, 6, 12, 0.62)",
        backdropFilter: "blur(14px)", WebkitBackdropFilter: "blur(14px)",
      }}
    >
      <GlassSurface
        elevated
        onClick={(e) => e.stopPropagation()}
        style={{
          width: "min(560px, calc(100vw - 48px))",
          maxHeight: "min(680px, calc(100vh - 48px))",
          borderRadius: 18, overflow: "hidden",
          display: "flex", flexDirection: "column",
        }}
      >
        {/* Header */}
        <div style={{
          padding: "16px 18px", display: "flex", alignItems: "center",
          gap: 12, borderBottom: "1px solid var(--divider)", flexShrink: 0,
        }}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div style={{ fontWeight: 700, fontSize: 15, lineHeight: 1.3, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
              {comic.title || "未命名"}
            </div>
            {comic.author && (
              <div style={{ fontSize: 12, color: "var(--text-secondary)", marginTop: 3 }}>
                {comic.author}
              </div>
            )}
          </div>
          <CloseButton onClose={onClose} />
        </div>

        {/* Chapter list */}
        <GlassScrollArea style={{ flex: 1, padding: "10px 12px 14px" }}>
          {chaptersQuery.isLoading && (
            <div style={{ display: "grid", placeItems: "center", padding: 40 }}>
              <Spinner size={26} />
            </div>
          )}
          {chaptersQuery.error && (
            <div style={{ padding: 20, color: "var(--color-badge)", fontSize: 13, textAlign: "center" }}>
              {chaptersQuery.error instanceof Error ? chaptersQuery.error.message : "加载失败"}
            </div>
          )}
          {!chaptersQuery.isLoading && !chaptersQuery.error && (chaptersQuery.data ?? []).length === 0 && (
            <div style={{ padding: 40, textAlign: "center", color: "var(--text-disabled)", fontSize: 13 }}>
              暂无章节
            </div>
          )}
          <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(150px, 1fr))", gap: 8 }}>
            {(chaptersQuery.data ?? []).map((ch) => (
              <button
                key={ch.chapter_id}
                onClick={() => onSelectChapter(ch.chapter_id, ch.title || `第 ${(ch.index ?? 0) + 1} 话`)}
                style={{
                  padding: "9px 12px", border: "1px solid var(--divider)",
                  borderRadius: 9, background: "var(--glass-btn-bg)",
                  color: "var(--text-primary)", fontSize: 13, textAlign: "left",
                  cursor: "pointer", transition: "background 120ms ease",
                  overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
                }}
                onMouseEnter={(e) => { e.currentTarget.style.background = "var(--tint-hover)" }}
                onMouseLeave={(e) => { e.currentTarget.style.background = "var(--glass-btn-bg)" }}
              >
                {ch.title || `第 ${(ch.index ?? 0) + 1} 话`}
              </button>
            ))}
          </div>
        </GlassScrollArea>
      </GlassSurface>
    </div>
  )
}

// ─── Image reader overlay ─────────────────────────────────────────────────

function ReaderPage({ url }: { url: string }) {
  const loaded = useMediaUrl(url)
  if (!loaded) return (
    <div style={{ width: "100%", minHeight: 200, display: "grid", placeItems: "center", background: "rgba(255,255,255,0.04)", borderRadius: 8 }}>
      <Spinner size={22} />
    </div>
  )
  return (
    <img
      src={loaded}
      alt=""
      loading="lazy"
      style={{ width: "100%", display: "block", borderRadius: 8 }}
    />
  )
}

function ReaderOverlay({
  comic,
  sourceId,
  chapterId,
  chapterTitle,
  onClose,
}: {
  comic: R18ComicItem
  sourceId: string
  chapterId: string
  chapterTitle: string
  onClose: () => void
}) {
  const imagesQuery = useQuery({
    queryKey: ["r18", "images", sourceId, comic.comic_id, chapterId],
    enabled: Boolean(sourceId && comic.comic_id && chapterId),
    queryFn: async () => {
      const res = await getGateway().http.post<R18ChapterImagesResponse>(ENDPOINTS.r18ChapterImages, {
        source_id: sourceId,
        comic_id: comic.comic_id,
        chapter_id: chapterId,
      })
      if (res.error !== 0) throw new Error(res.message || `获取图片失败: ${res.error}`)
      return res.data?.images ?? []
    },
  })

  return (
    <div
      role="dialog"
      aria-modal="true"
      style={{
        position: "fixed", inset: 0, zIndex: 95,
        display: "flex", flexDirection: "column",
        background: "rgba(2, 6, 12, 0.88)",
        backdropFilter: "blur(18px)", WebkitBackdropFilter: "blur(18px)",
      }}
    >
      {/* Reader topbar */}
      <div style={{
        display: "flex", alignItems: "center", gap: 12,
        padding: "10px 18px", borderBottom: "1px solid rgba(255,255,255,0.10)",
        background: "rgba(6,10,16,0.60)", flexShrink: 0,
      }}>
        <button
          type="button"
          onClick={onClose}
          aria-label="返回"
          style={{
            display: "flex", alignItems: "center", gap: 6,
            padding: "6px 12px", borderRadius: 9,
            border: "1px solid rgba(255,255,255,0.14)",
            background: "rgba(255,255,255,0.08)", color: "#dde4e2",
            cursor: "pointer", fontSize: 13, flexShrink: 0,
          }}
        >
          ← 返回
        </button>
        <div style={{ flex: 1, minWidth: 0 }}>
          <div style={{ fontSize: 14, fontWeight: 600, color: "#f0f3f0", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
            {comic.title || "未命名"} · {chapterTitle}
          </div>
        </div>
      </div>

      {/* Page list */}
      <GlassScrollArea style={{ flex: 1 }}>
        <div style={{ maxWidth: 820, margin: "0 auto", padding: "20px 16px 32px", display: "flex", flexDirection: "column", gap: 12 }}>
          {imagesQuery.isLoading && (
            <div style={{ display: "grid", placeItems: "center", padding: 60 }}>
              <Spinner size={30} />
            </div>
          )}
          {imagesQuery.error && (
            <div style={{ padding: 40, color: "var(--color-badge)", fontSize: 14, textAlign: "center" }}>
              {imagesQuery.error instanceof Error ? imagesQuery.error.message : "加载失败"}
            </div>
          )}
          {(imagesQuery.data ?? []).map((url, idx) => (
            <ReaderPage key={`${chapterId}-${idx}`} url={url} />
          ))}
          {!imagesQuery.isLoading && !imagesQuery.error && (imagesQuery.data ?? []).length === 0 && (
            <div style={{ padding: 60, textAlign: "center", color: "#aeb5bd", fontSize: 14 }}>
              本章暂无图片
            </div>
          )}
        </div>
      </GlassScrollArea>
    </div>
  )
}

// ─── Main component ───────────────────────────────────────────────────────

export function R18ShellContent() {
  const uid = useSessionStore((s) => s.uid)
  const token = useSessionStore((s) => s.token)
  const [selectedSourceId, setSelectedSourceId] = useState("")
  const [keyword, setKeyword] = useState("")
  const [submittedKeyword, setSubmittedKeyword] = useState("")
  const [page, setPage] = useState(1)
  const [actionError, setActionError] = useState<string | null>(null)

  /** Comic whose chapter list is open */
  const [comicForChapters, setComicForChapters] = useState<R18ComicItem | null>(null)
  /** Chapter being read */
  const [readerState, setReaderState] = useState<{
    comic: R18ComicItem
    chapterId: string
    chapterTitle: string
  } | null>(null)

  const authReady = uid !== null && token !== null

  const sourcesQuery = useQuery({
    queryKey: ["r18", "sources", uid],
    enabled: authReady,
    queryFn: async () => {
      if (uid === null || token === null) throw new Error("Missing R18 auth")
      const res = await getGateway().http.get<R18SourcesResponse>(ENDPOINTS.r18Sources)
      if (res.error !== 0) throw new Error(res.message || `R18 sources failed: ${res.error}`)
      return res.data?.sources ?? []
    },
  })

  const sources = useMemo(() => sourcesQuery.data ?? [], [sourcesQuery.data])
  const selectedSource = useMemo(
    () => sources.find((s) => s.id === selectedSourceId) ?? sources.find((s) => s.enabled) ?? sources[0],
    [sources, selectedSourceId],
  )

  useEffect(() => {
    if (!selectedSourceId && selectedSource?.id) setSelectedSourceId(selectedSource.id)
  }, [selectedSource?.id, selectedSourceId])

  const searchQuery = useQuery({
    queryKey: ["r18", "search", uid, selectedSource?.id, submittedKeyword, page],
    enabled: authReady && Boolean(selectedSource?.id),
    queryFn: async () => {
      if (uid === null || token === null || !selectedSource?.id) throw new Error("Missing R18 search input")
      const res = await getGateway().http.post<R18SearchResponse>(ENDPOINTS.r18Search, {
        source_id: selectedSource.id,
        keyword: submittedKeyword,
        page,
      })
      if (res.error !== 0) throw new Error(res.message || `R18 search failed: ${res.error}`)
      return res.data ?? { items: [] }
    },
  })

  async function toggleSource(source: R18Source) {
    if (uid === null || token === null) return
    setActionError(null)
    try {
      const res = await getGateway().http.post<{ error: number; message?: string }>(
        source.enabled ? ENDPOINTS.r18SourceDisable : ENDPOINTS.r18SourceEnable,
        { source_id: source.id },
      )
      if (res.error !== 0) throw new Error(res.message || "源切换失败")
      await sourcesQuery.refetch()
    } catch (err) {
      setActionError(err instanceof Error ? err.message : "源切换失败")
    }
  }

  function submitSearch() {
    setSubmittedKeyword(keyword.trim())
    setPage(1)
  }

  // ── Reader overlay (full-screen) ────────────────────────────────
  if (readerState) {
    return (
      <ReaderOverlay
        comic={readerState.comic}
        sourceId={selectedSource?.id ?? ""}
        chapterId={readerState.chapterId}
        chapterTitle={readerState.chapterTitle}
        onClose={() => setReaderState(null)}
      />
    )
  }

  if (!authReady) {
    return (
      <div style={{ height: "100%", width: "100%", display: "grid", placeItems: "center", color: "var(--text-disabled)" }}>
        登录状态无效
      </div>
    )
  }

  return (
    <div style={{ height: "100%", width: "100%", minWidth: 0, display: "grid", gridTemplateColumns: "260px minmax(0, 1fr)", overflow: "hidden" }}>

      {/* ── Source list sidebar ─────────────────────────────────── */}
      <aside style={{ borderRight: "1px solid var(--divider)", display: "flex", flexDirection: "column", minHeight: 0 }}>
        <div style={{ padding: "14px 14px 10px", flexShrink: 0 }}>
          <div style={{ fontSize: 16, fontWeight: 700 }}>R18</div>
          <div style={{ marginTop: 3, fontSize: 12, color: "var(--text-secondary)" }}>内容源</div>
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
                onClick={() => { setSelectedSourceId(source.id); setPage(1) }}
                style={{
                  width: "100%", border: "none", borderRadius: 10,
                  padding: "10px 12px", marginBottom: 4, textAlign: "left",
                  cursor: "pointer", transition: "background 120ms ease",
                  background: active ? "var(--tint-selected)" : "transparent",
                  color: "var(--text-primary)",
                }}
                onMouseEnter={(e) => { if (!active) e.currentTarget.style.background = "var(--tint-hover)" }}
                onMouseLeave={(e) => { if (!active) e.currentTarget.style.background = "transparent" }}
              >
                <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", gap: 8 }}>
                  <span style={{ minWidth: 0, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", fontWeight: 600, fontSize: 14 }}>
                    {sourceTitle(source)}
                  </span>
                  <span style={{
                    flexShrink: 0, width: 8, height: 8, borderRadius: "50%",
                    background: source.enabled ? "var(--color-brand-green)" : "var(--text-disabled)",
                  }} />
                </div>
                <div style={{ marginTop: 3, fontSize: 12, color: "var(--text-secondary)" }}>
                  {source.version || source.status || source.id}
                </div>
              </button>
            )
          })}
        </GlassScrollArea>
      </aside>

      {/* ── Main content area ───────────────────────────────────── */}
      <section style={{ minWidth: 0, display: "flex", flexDirection: "column", overflow: "hidden" }}>
        {/* Search bar */}
        <div style={{ padding: "12px 16px", borderBottom: "1px solid var(--divider)", display: "flex", gap: 8, alignItems: "center", flexShrink: 0 }}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <GlassTextField
              value={keyword}
              onChange={(e) => setKeyword(e.target.value)}
              placeholder="搜索关键词…"
              onKeyDown={(e) => { if (e.key === "Enter") { e.preventDefault(); submitSearch() } }}
            />
          </div>
          <GlassButton onClick={submitSearch} variant="primary" style={{ flexShrink: 0 }}>搜索</GlassButton>
          {selectedSource && (
            <GlassButton onClick={() => void toggleSource(selectedSource)} style={{ flexShrink: 0 }}>
              {selectedSource.enabled ? "停用源" : "启用源"}
            </GlassButton>
          )}
        </div>

        {/* Pagination row */}
        <div style={{ display: "flex", alignItems: "center", gap: 8, padding: "8px 16px", borderBottom: "1px solid var(--divider)", flexShrink: 0 }}>
          <GlassButton
            onClick={() => setPage((p) => Math.max(1, p - 1))}
            disabled={page <= 1}
            style={{ padding: "5px 12px", fontSize: 12 }}
          >
            ← 上一页
          </GlassButton>
          <span style={{ fontSize: 13, color: "var(--text-secondary)", flex: 1, textAlign: "center" }}>第 {page} 页</span>
          <GlassButton
            onClick={() => setPage((p) => p + 1)}
            disabled={!searchQuery.data?.max_page || page >= (searchQuery.data.max_page ?? 1)}
            style={{ padding: "5px 12px", fontSize: 12 }}
          >
            下一页 →
          </GlassButton>
        </div>

        {(actionError || sourcesQuery.error || searchQuery.error) && (
          <div style={{ margin: "10px 16px 0", color: "var(--color-badge)", fontSize: 13 }}>
            {actionError ||
              (sourcesQuery.error instanceof Error ? sourcesQuery.error.message : null) ||
              (searchQuery.error instanceof Error ? searchQuery.error.message : "加载失败")}
          </div>
        )}

        {/* Comic grid */}
        <GlassScrollArea style={{ flex: 1, minHeight: 0, padding: 16 }}>
          {searchQuery.isLoading ? (
            <div style={{ display: "grid", placeItems: "center", height: 200 }}>
              <Spinner size={28} />
            </div>
          ) : (
            <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(170px, 1fr))", gap: 12 }}>
              {(searchQuery.data?.items ?? []).map((item) => (
                <GlassSurface
                  key={`${item.source_id}-${item.comic_id}`}
                  elevated
                  style={{
                    overflow: "hidden", cursor: "pointer",
                    transition: "transform 150ms ease, box-shadow 150ms ease",
                  }}
                  onClick={() => setComicForChapters(item)}
                  onMouseEnter={(e) => {
                    const el = e.currentTarget as HTMLElement
                    el.style.transform = "translateY(-3px)"
                  }}
                  onMouseLeave={(e) => {
                    const el = e.currentTarget as HTMLElement
                    el.style.transform = ""
                  }}
                  tabIndex={0}
                  role="button"
                  onKeyDown={(e) => { if (e.key === "Enter" || e.key === " ") { e.preventDefault(); setComicForChapters(item) } }}
                >
                  <R18Cover
                    {...(item.cover !== undefined ? { cover: item.cover } : {})}
                    {...(item.title !== undefined ? { title: item.title } : {})}
                  />
                  <div style={{ padding: "10px 11px 12px" }}>
                    <div style={{
                      fontWeight: 700, fontSize: 13, lineHeight: 1.35,
                      overflow: "hidden", display: "-webkit-box",
                      WebkitLineClamp: 2, WebkitBoxOrient: "vertical",
                    }}>
                      {item.title || item.comic_id || "未命名"}
                    </div>
                    {(item.subtitle || item.author) && (
                      <div style={{ marginTop: 5, fontSize: 11, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
                        {item.subtitle || item.author}
                      </div>
                    )}
                  </div>
                </GlassSurface>
              ))}

              {(searchQuery.data?.items ?? []).length === 0 && !searchQuery.isLoading && (
                <div style={{
                  gridColumn: "1 / -1", padding: 60, textAlign: "center",
                  color: "var(--text-disabled)", fontSize: 14,
                }}>
                  暂无结果，请搜索关键词
                </div>
              )}
            </div>
          )}
        </GlassScrollArea>
      </section>

      {/* ── Chapter list overlay ────────────────────────────────── */}
      {comicForChapters && (
        <ChapterListOverlay
          comic={comicForChapters}
          sourceId={selectedSource?.id ?? ""}
          onClose={() => setComicForChapters(null)}
          onSelectChapter={(chapterId, chapterTitle) => {
            setReaderState({ comic: comicForChapters, chapterId, chapterTitle })
            setComicForChapters(null)
          }}
        />
      )}
    </div>
  )
}
