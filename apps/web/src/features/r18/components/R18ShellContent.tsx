/** R18ShellContent — source switcher + search + chapter reader */
import { useEffect, useMemo, useRef, useState } from "react"
import { useMutation, useQuery } from "@tanstack/react-query"
import { useSessionStore } from "@/core/session/sessionStore"
import {
  createR18Api,
  type R18ComicItem,
  type R18Source,
} from "@/features/r18/api/r18Api"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { useMediaUrl } from "@/shared/hooks/useMediaUrl"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { Spinner } from "@/shared/ui/primitives/Spinner"
import styles from "./R18ShellContent.module.css"

// ─── Types ────────────────────────────────────────────────────────────────

// ─── Helpers ──────────────────────────────────────────────────────────────

function sourceTitle(source: R18Source): string {
  return source.title?.trim() || source.name?.trim() || source.id
}

const DIALOG_FOCUSABLE_SELECTOR = [
  "button:not([disabled])",
  "a[href]",
  "input:not([disabled])",
  "select:not([disabled])",
  "textarea:not([disabled])",
  '[tabindex]:not([tabindex="-1"])',
].join(",")

function useDialogAccessibility(onClose: () => void) {
  const dialogRef = useRef<HTMLDivElement>(null)
  const onCloseRef = useRef(onClose)

  useEffect(() => {
    onCloseRef.current = onClose
  }, [onClose])

  useEffect(() => {
    const dialog = dialogRef.current
    if (!dialog) return

    const previousFocus = document.activeElement instanceof HTMLElement
      ? document.activeElement
      : null
    const focusableElements = () => Array.from(
      dialog.querySelectorAll<HTMLElement>(DIALOG_FOCUSABLE_SELECTOR),
    ).filter((element) => !element.hidden && element.getAttribute("aria-hidden") !== "true")

    const initialFocus = focusableElements()[0] ?? dialog
    initialFocus.focus()

    function handleKeyDown(event: KeyboardEvent) {
      if (event.key === "Escape") {
        event.preventDefault()
        onCloseRef.current()
        return
      }
      if (event.key !== "Tab") return

      const focusable = focusableElements()
      if (focusable.length === 0) {
        event.preventDefault()
        dialog?.focus()
        return
      }

      const first = focusable[0]
      const last = focusable[focusable.length - 1]
      if (!first || !last) return
      const active = document.activeElement
      if (event.shiftKey && (active === first || !dialog?.contains(active))) {
        event.preventDefault()
        last.focus()
      } else if (!event.shiftKey && active === last) {
        event.preventDefault()
        first.focus()
      }
    }

    document.addEventListener("keydown", handleKeyDown)
    return () => {
      document.removeEventListener("keydown", handleKeyDown)
      if (previousFocus?.isConnected) previousFocus.focus()
    }
  }, [])

  return dialogRef
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
        background: "var(--glass-btn-bg)", color: "var(--text-secondary)",
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
  const dialogRef = useDialogAccessibility(onClose)
  const api = useMemo(() => createR18Api(getGateway().http), [])
  const chaptersQuery = useQuery({
    queryKey: ["r18", "chapters", sourceId, comic.comic_id],
    enabled: Boolean(sourceId && comic.comic_id),
    queryFn: async () => {
      return api.listChapters(sourceId, comic.comic_id ?? "")
    },
  })

  return (
    <div
      ref={dialogRef}
      role="dialog"
      aria-modal="true"
      aria-labelledby="r18-chapter-dialog-title"
      tabIndex={-1}
      onClick={onClose}
      style={{
        position: "fixed", inset: 0, zIndex: 90,
        display: "grid", placeItems: "center", padding: 24,
        background: "var(--r18-dialog-backdrop)",
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
            <div id="r18-chapter-dialog-title" style={{ fontWeight: 700, fontSize: 15, lineHeight: 1.3, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
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
    <div style={{ width: "100%", minHeight: 200, display: "grid", placeItems: "center", background: "var(--tint-hover)", borderRadius: 8 }}>
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
  const dialogRef = useDialogAccessibility(onClose)
  const api = useMemo(() => createR18Api(getGateway().http), [])
  const imagesQuery = useQuery({
    queryKey: ["r18", "images", sourceId, comic.comic_id, chapterId],
    enabled: Boolean(sourceId && comic.comic_id && chapterId),
    queryFn: async () => {
      return api.listPageUrls(sourceId, chapterId)
    },
  })

  return (
    <div
      ref={dialogRef}
      role="dialog"
      aria-modal="true"
      aria-labelledby="r18-reader-dialog-title"
      tabIndex={-1}
      style={{
        position: "fixed", inset: 0, zIndex: 95,
        display: "flex", flexDirection: "column",
        background: "var(--r18-reader-backdrop)",
        backdropFilter: "blur(18px)", WebkitBackdropFilter: "blur(18px)",
      }}
    >
      {/* Reader topbar */}
      <div style={{
        display: "flex", alignItems: "center", gap: 12,
        padding: "10px 18px", borderBottom: "1px solid var(--divider)",
        background: "var(--r18-reader-toolbar-bg)", flexShrink: 0,
      }}>
        <button
          type="button"
          onClick={onClose}
          aria-label="返回"
          style={{
            display: "flex", alignItems: "center", gap: 6,
            padding: "6px 12px", borderRadius: 9,
            border: "1px solid var(--divider)",
            background: "var(--glass-btn-bg)", color: "var(--text-primary)",
            cursor: "pointer", fontSize: 13, flexShrink: 0,
          }}
        >
          ← 返回
        </button>
        <div style={{ flex: 1, minWidth: 0 }}>
          <div id="r18-reader-dialog-title" style={{ fontSize: 14, fontWeight: 600, color: "var(--text-primary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
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
            <div style={{ padding: 60, textAlign: "center", color: "var(--text-secondary)", fontSize: 14 }}>
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

  /** Comic whose chapter list is open */
  const [comicForChapters, setComicForChapters] = useState<R18ComicItem | null>(null)
  /** Chapter being read */
  const [readerState, setReaderState] = useState<{
    comic: R18ComicItem
    chapterId: string
    chapterTitle: string
  } | null>(null)

  const authReady = uid !== null && token !== null
  const api = useMemo(() => createR18Api(getGateway().http), [])

  const accessQuery = useQuery({
    queryKey: ["r18", "access", uid],
    enabled: authReady,
    retry: false,
    queryFn: async () => {
      if (!authReady) throw new Error("Missing R18 auth")
      return api.getAccess()
    },
  })

  const attestMutation = useMutation({
    mutationFn: () => api.attestAdult(),
  })

  const sourcesQuery = useQuery({
    queryKey: ["r18", "sources", uid],
    enabled: authReady && accessQuery.data?.allowed === true,
    queryFn: async () => {
      if (!authReady || accessQuery.data?.allowed !== true) throw new Error("R18 access denied")
      return api.listSources()
    },
  })

  const sources = useMemo(() => sourcesQuery.data ?? [], [sourcesQuery.data])
  const selectedSource = useMemo(
    () => sources.find((s) => s.id === selectedSourceId && s.enabled) ?? sources.find((s) => s.enabled),
    [sources, selectedSourceId],
  )

  useEffect(() => {
    if (!selectedSourceId && selectedSource?.id) setSelectedSourceId(selectedSource.id)
  }, [selectedSource?.id, selectedSourceId])

  const searchQuery = useQuery({
    queryKey: ["r18", "search", uid, selectedSource?.id, submittedKeyword, page],
    enabled: authReady && accessQuery.data?.allowed === true && Boolean(selectedSource?.id),
    queryFn: async () => {
      if (!authReady || accessQuery.data?.allowed !== true || !selectedSource?.id) {
        throw new Error("Missing R18 search input")
      }
      return api.search(selectedSource.id, submittedKeyword, page)
    },
  })

  async function attestR18Access() {
    try {
      await attestMutation.mutateAsync()
    } finally {
      await accessQuery.refetch()
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

  if (accessQuery.isLoading) {
    return (
      <div style={{ height: "100%", width: "100%", display: "grid", placeItems: "center" }}>
        <Spinner size={28} />
      </div>
    )
  }

  if (accessQuery.error) {
    return (
      <div style={{ height: "100%", width: "100%", display: "grid", placeItems: "center", padding: 24 }}>
        <div style={{ display: "grid", justifyItems: "center", gap: 14, color: "var(--text-secondary)", textAlign: "center" }}>
          <div>访问策略暂时不可用</div>
          <GlassButton onClick={() => void accessQuery.refetch()}>重试</GlassButton>
        </div>
      </div>
    )
  }

  if (accessQuery.data?.allowed !== true) {
    const revoked = accessQuery.data?.state === "revoked"
    return (
      <div style={{ height: "100%", width: "100%", display: "grid", placeItems: "center", padding: 24 }}>
        <div style={{ display: "grid", justifyItems: "center", gap: 14, maxWidth: 420, textAlign: "center" }}>
          <div style={{ fontSize: 17, fontWeight: 700, color: "var(--text-primary)" }}>
            {revoked ? "访问权限已被撤销" : "成人内容访问确认"}
          </div>
          {!revoked && (
            <GlassButton
              variant="primary"
              disabled={attestMutation.isPending}
              onClick={() => void attestR18Access()}
            >
              我已年满 18 岁
            </GlassButton>
          )}
          {attestMutation.error && (
            <div style={{ color: "var(--color-badge)", fontSize: 13 }}>
              {attestMutation.error instanceof Error ? attestMutation.error.message : "确认失败"}
            </div>
          )}
        </div>
      </div>
    )
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

  return (
    <div className={styles.shell}>

      {/* ── Source list sidebar ─────────────────────────────────── */}
      <aside className={styles.sourceSidebar}>
        <div className={styles.sourceHeader}>
          <div style={{ fontSize: 16, fontWeight: 700 }}>R18</div>
          <div style={{ marginTop: 3, fontSize: 12, color: "var(--text-secondary)" }}>内容源</div>
        </div>
        <GlassScrollArea className={styles.sourceList ?? ""}>
          {sourcesQuery.isLoading ? (
            <div style={{ display: "grid", placeItems: "center", padding: 28 }}>
              <Spinner size={24} />
            </div>
          ) : sources.map((source) => {
            const active = source.id === selectedSource?.id
            return (
              <button
                key={source.id}
                className={styles.sourceButton}
                disabled={!source.enabled}
                onClick={() => { setSelectedSourceId(source.id); setPage(1) }}
                style={{
                  border: "none", borderRadius: 10,
                  padding: "10px 12px", textAlign: "left",
                  cursor: source.enabled ? "pointer" : "not-allowed", transition: "background 120ms ease",
                  background: active ? "var(--tint-selected)" : "transparent",
                  color: source.enabled ? "var(--text-primary)" : "var(--text-disabled)",
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
        <div className={styles.searchBar}>
          <div className={styles.searchInput}>
            <GlassTextField
              value={keyword}
              onChange={(e) => setKeyword(e.target.value)}
              placeholder="搜索关键词…"
              onKeyDown={(e) => { if (e.key === "Enter") { e.preventDefault(); submitSearch() } }}
            />
          </div>
          <GlassButton onClick={submitSearch} variant="primary" style={{ flexShrink: 0 }}>搜索</GlassButton>
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

        {(sourcesQuery.error || searchQuery.error) && (
          <div style={{ margin: "10px 16px 0", color: "var(--color-badge)", fontSize: 13 }}>
            {(sourcesQuery.error instanceof Error ? sourcesQuery.error.message : null) ||
              (searchQuery.error instanceof Error ? searchQuery.error.message : "加载失败")}
          </div>
        )}

        {!sourcesQuery.isLoading && !sourcesQuery.error && !selectedSource && (
          <div style={{ margin: "10px 16px 0", color: "var(--text-secondary)", fontSize: 13 }}>
            暂无可用内容源
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
