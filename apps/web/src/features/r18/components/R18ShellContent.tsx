/** R18ShellContent — source switcher + search + chapter reader */
import { useEffect, useMemo, useRef, useState } from "react"
import { useMutation, useQuery } from "@tanstack/react-query"
import { useSessionStore } from "@/core/session/sessionStore"
import {
  createR18Api,
  type R18ComicItem,
  type R18ManagedAccount,
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
import { accountInteractionKind, isActionableSource } from "./r18SourceAvailability"

// ─── Types ────────────────────────────────────────────────────────────────

// ─── Helpers ──────────────────────────────────────────────────────────────

function sourceTitle(source: R18Source): string {
  return source.title?.trim() || source.name?.trim() || source.id
}

function sourceStatusLabel(source: R18Source): string {
  if (source.enabled) {
    if (source.direct_access) return "可直接访问"
    if (source.account_status === "authenticated") return "已登录"
    return source.version || source.status || source.id
  }
  if (source.status === "auth-required" || source.status === "credentials-missing") {
    return "需要登录"
  }
  if (source.status === "staged-js") {
    return "JS 源待运行时接入"
  }
  const message = source.message?.trim() ?? ""
  if (
    message === "Picacg credentials missing" ||
    message === "Picacg account login required"
  ) {
    return "需要登录"
  }
  if (message.includes("source runtime adapter")) {
    return "JS 源待运行时接入"
  }
  if (message) {
    return message
  }
  return source.status || "不可用"
}

function humanizeR18Error(message?: string | null): string {
  const text = message?.trim() ?? ""
  if (!text) {
    return "加载失败"
  }
  if (text === "Picacg credentials missing" || text === "Picacg account login required") {
    return "哔咔源需要账号登录，请在左侧「账号管理」中保存账号密码"
  }
  if (text.startsWith("Picacg login token missing") || text.startsWith("Picacg login returned success without a session")) {
    return "哔咔上游未返回登录会话，请检查签名配置或稍后重试"
  }
  if (text.startsWith("Picacg login failed")) {
    return text.replace("Picacg login failed:", "哔咔登录失败：")
  }
  if (text === "source is disabled") {
    return "当前内容源已禁用"
  }
  if (text === "source not found") {
    return "内容源不存在"
  }
  return text
}

function isSourceFailureItem(item: R18ComicItem): boolean {
  const title = item.title?.trim() ?? ""
  return !item.comic_id && (title === "官方源请求失败" || title === "内容源请求失败")
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

// ─── Account manager ─────────────────────────────────────────────────────

function accountStatusLabel(account: R18ManagedAccount): string {
  const interaction = accountInteractionKind(account)
  if (interaction === "optional-account") {
    if (account.status === "authenticated" && account.has_session) return "已登录"
    return "可直接访问，也可登录账号"
  }
  if (interaction === "optional-cookie") {
    if (account.status === "authenticated" && account.has_session) return "可选 Cookie 已配置"
    return "可直接访问，也可配置 Cookie"
  }
  switch (account.status) {
    case "authenticated":
      return "已登录"
    case "configured":
      return "已保存，待登录"
    case "error":
      return account.message?.trim() || "登录失败"
    case "not_configured":
      return account.auth_required ? "未配置账号" : "无需账号"
    case "direct-access":
      return "无需账号，可直接访问"
    default:
      return account.message?.trim() || account.status || "未知状态"
  }
}

function AccountManagerPanel({
  accounts,
  busySourceId,
  drafts,
  onDraftChange,
  onSave,
  onLogin,
  onClear,
  onClose,
}: {
  accounts: R18ManagedAccount[]
  busySourceId: string | null
  drafts: Record<string, { username: string; password: string }>
  onDraftChange: (sourceId: string, field: "username" | "password", value: string) => void
  onSave: (sourceId: string) => void
  onLogin: (sourceId: string) => void
  onClear: (sourceId: string) => void
  onClose: () => void
}) {
  const dialogRef = useDialogAccessibility(onClose)
  return (
    <div
      ref={dialogRef}
      role="dialog"
      aria-modal="true"
      aria-labelledby="r18-account-dialog-title"
      tabIndex={-1}
      onClick={onClose}
      style={{
        position: "fixed", inset: 0, zIndex: 92,
        display: "grid", placeItems: "center", padding: 24,
        background: "var(--r18-dialog-backdrop)",
        backdropFilter: "blur(14px)", WebkitBackdropFilter: "blur(14px)",
      }}
    >
      <GlassSurface
        elevated
        onClick={(e) => e.stopPropagation()}
        style={{
          width: "min(640px, calc(100vw - 48px))",
          maxHeight: "min(760px, calc(100vh - 48px))",
          borderRadius: 18, overflow: "hidden",
          display: "flex", flexDirection: "column",
        }}
      >
        <div style={{
          padding: "16px 18px", display: "flex", alignItems: "center",
          gap: 12, borderBottom: "1px solid var(--divider)", flexShrink: 0,
        }}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div id="r18-account-dialog-title" style={{ fontWeight: 700, fontSize: 15 }}>
              内容源账号管理
            </div>
            <div style={{ marginTop: 4, fontSize: 12, color: "var(--text-secondary)" }}>
              无需账号的源可直接访问；需要账号的源填写后自动保存并登录
            </div>
          </div>
          <CloseButton onClose={onClose} />
        </div>

        <GlassScrollArea style={{ flex: 1, padding: "12px 14px 16px" }}>
          <div style={{ display: "grid", gap: 12 }}>
            {accounts.map((account) => {
              const sourceId = account.source_id
              const draft = drafts[sourceId] ?? {
                username: account.username ?? "",
                password: "",
              }
              const busy = busySourceId === sourceId
              const interaction = accountInteractionKind(account)
              const needsAccount = interaction === "required-account"
              const optionalAccount = interaction === "optional-account"
              const optionalCookie = interaction === "optional-cookie"
              const supportsCredentials = interaction !== "none"
              return (
                <GlassSurface
                  key={sourceId}
                  style={{
                    padding: 14,
                    borderRadius: 14,
                    border: "1px solid var(--divider)",
                    display: "grid",
                    gap: 10,
                  }}
                >
                  <div style={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", gap: 10 }}>
                    <div style={{ minWidth: 0 }}>
                      <div style={{ fontWeight: 700, fontSize: 14 }}>
                        {account.name || sourceId}
                      </div>
                      <div style={{ marginTop: 3, fontSize: 12, color: "var(--text-secondary)" }}>
                        {accountStatusLabel(account)}
                      </div>
                      {account.message && account.status === "error" && (
                        <div style={{ marginTop: 4, fontSize: 12, color: "var(--color-badge)" }}>
                          {account.message}
                        </div>
                      )}
                    </div>
                    <span style={{
                      flexShrink: 0,
                      fontSize: 11,
                      padding: "3px 8px",
                      borderRadius: 999,
                      border: "1px solid var(--divider)",
                      color: needsAccount ? "var(--text-primary)" : "var(--text-secondary)",
                      background: "var(--glass-btn-bg)",
                    }}>
                      {needsAccount ? "需要账号" : optionalAccount ? "可选账号" : optionalCookie ? "可选 Cookie" : "直接访问"}
                    </span>
                  </div>

                  {supportsCredentials ? (
                    <>
                      <GlassTextField
                        value={draft.username}
                        onChange={(e) => onDraftChange(sourceId, "username", e.target.value)}
                        placeholder={optionalCookie ? "备注名（可选）" : "账号 / 邮箱"}
                        autoComplete="username"
                      />
                      <GlassTextField
                        type="password"
                        value={draft.password}
                        onChange={(e) => onDraftChange(sourceId, "password", e.target.value)}
                        placeholder={optionalCookie
                          ? "可选 Cookie（ipb_member_id=...; ipb_pass_hash=...）"
                          : (account.has_password ? "密码（留空则保留已保存密码）" : "密码")}
                        autoComplete="current-password"
                      />
                      <div style={{ display: "flex", flexWrap: "wrap", gap: 8 }}>
                        <GlassButton
                          variant="primary"
                          disabled={busy}
                          onClick={() => onSave(sourceId)}
                          style={{ fontSize: 12 }}
                        >
                          {busy ? "处理中…" : "保存并登录"}
                        </GlassButton>
                        <GlassButton
                          disabled={busy || (!account.has_password && !draft.password && !account.has_session)}
                          onClick={() => onLogin(sourceId)}
                          style={{ fontSize: 12 }}
                        >
                          重新登录
                        </GlassButton>
                        <GlassButton
                          disabled={busy || (!account.has_password && !account.has_session && !(account.username?.trim()))}
                          onClick={() => onClear(sourceId)}
                          style={{ fontSize: 12 }}
                        >
                          清除
                        </GlassButton>
                      </div>
                    </>
                  ) : (
                    <div style={{ fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.5 }}>
                      此源无需账号密码，可在左侧直接选择后搜索浏览。
                    </div>
                  )}
                </GlassSurface>
              )
            })}
            {accounts.length === 0 && (
              <div style={{ padding: 28, textAlign: "center", color: "var(--text-disabled)", fontSize: 13 }}>
                暂无托管账号项
              </div>
            )}
          </div>
        </GlassScrollArea>
      </GlassSurface>
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
  const [accountPanelOpen, setAccountPanelOpen] = useState(false)
  const [accountDrafts, setAccountDrafts] = useState<Record<string, { username: string; password: string }>>({})
  const [accountBusySourceId, setAccountBusySourceId] = useState<string | null>(null)
  const [accountActionError, setAccountActionError] = useState<string | null>(null)

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

  const accountsQuery = useQuery({
    queryKey: ["r18", "accounts", uid],
    enabled: authReady && accessQuery.data?.allowed === true,
    queryFn: async () => {
      if (!authReady || accessQuery.data?.allowed !== true) throw new Error("R18 access denied")
      return api.listAccounts()
    },
  })

  const sources = useMemo(
    () => (sourcesQuery.data ?? []).filter(isActionableSource),
    [sourcesQuery.data],
  )
  const managedAccounts = useMemo(
    () => (accountsQuery.data?.managed ?? accountsQuery.data?.accounts ?? [])
      .filter((account) => accountInteractionKind(account) !== "none"),
    [accountsQuery.data],
  )
  const selectedSource = useMemo(
    () => sources.find((s) => s.id === selectedSourceId && s.enabled) ?? sources.find((s) => s.enabled),
    [sources, selectedSourceId],
  )

  useEffect(() => {
    if (!selectedSourceId && selectedSource?.id) setSelectedSourceId(selectedSource.id)
  }, [selectedSource?.id, selectedSourceId])

  useEffect(() => {
    if (!accountPanelOpen || managedAccounts.length === 0) return
    setAccountDrafts((prev) => {
      const next = { ...prev }
      for (const account of managedAccounts) {
        if (!next[account.source_id]) {
          next[account.source_id] = {
            username: account.username ?? "",
            password: "",
          }
        }
      }
      return next
    })
  }, [accountPanelOpen, managedAccounts])

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

  function updateAccountDraft(sourceId: string, field: "username" | "password", value: string) {
    setAccountDrafts((prev) => ({
      ...prev,
      [sourceId]: {
        username: field === "username" ? value : (prev[sourceId]?.username ?? ""),
        password: field === "password" ? value : (prev[sourceId]?.password ?? ""),
      },
    }))
  }

  async function refreshAccountViews() {
    await Promise.all([accountsQuery.refetch(), sourcesQuery.refetch()])
  }

  async function saveAccount(sourceId: string) {
    const draft = accountDrafts[sourceId] ?? { username: "", password: "" }
    setAccountBusySourceId(sourceId)
    setAccountActionError(null)
    try {
      const payload = await api.saveAccount(sourceId, draft.username.trim(), draft.password)
      setAccountDrafts((prev) => ({
        ...prev,
        [sourceId]: { username: draft.username.trim(), password: "" },
      }))
      await refreshAccountViews()
      // Prefer server-managed payload when present.
      if (payload.managed) {
        // no-op; query refetch already applied
      }
    } catch (error) {
      setAccountActionError(error instanceof Error ? error.message : "保存账号失败")
    } finally {
      setAccountBusySourceId(null)
    }
  }

  async function loginAccount(sourceId: string) {
    const draft = accountDrafts[sourceId]
    setAccountBusySourceId(sourceId)
    setAccountActionError(null)
    try {
      await api.loginAccount(sourceId, draft?.username?.trim() || undefined, draft?.password || undefined)
      setAccountDrafts((prev) => ({
        ...prev,
        [sourceId]: {
          username: draft?.username?.trim() ?? prev[sourceId]?.username ?? "",
          password: "",
        },
      }))
      await refreshAccountViews()
    } catch (error) {
      setAccountActionError(error instanceof Error ? error.message : "登录失败")
    } finally {
      setAccountBusySourceId(null)
    }
  }

  async function clearAccount(sourceId: string) {
    setAccountBusySourceId(sourceId)
    setAccountActionError(null)
    try {
      await api.clearAccount(sourceId)
      setAccountDrafts((prev) => ({
        ...prev,
        [sourceId]: { username: "", password: "" },
      }))
      await refreshAccountViews()
    } catch (error) {
      setAccountActionError(error instanceof Error ? error.message : "清除账号失败")
    } finally {
      setAccountBusySourceId(null)
    }
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
          <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", gap: 8 }}>
            <div>
              <div style={{ fontSize: 16, fontWeight: 700 }}>R18</div>
              <div style={{ marginTop: 3, fontSize: 12, color: "var(--text-secondary)" }}>内容源</div>
            </div>
            <GlassButton
              onClick={() => {
                setAccountActionError(null)
                setAccountPanelOpen(true)
              }}
              style={{ padding: "5px 10px", fontSize: 12, flexShrink: 0 }}
            >
              账号
            </GlassButton>
          </div>
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
                disabled={!isActionableSource(source)}
                onClick={() => {
                  if (!source.enabled && isActionableSource(source)) {
                    setAccountActionError(null)
                    setAccountPanelOpen(true)
                    return
                  }
                  if (!source.enabled) return
                  setSelectedSourceId(source.id)
                  setPage(1)
                }}
                style={{
                  border: "none", borderRadius: 10,
                  padding: "10px 12px", textAlign: "left",
                  cursor: isActionableSource(source)
                    ? "pointer"
                    : "not-allowed",
                  transition: "background 120ms ease",
                  background: active ? "var(--tint-selected)" : "transparent",
                  color: isActionableSource(source) ? "var(--text-primary)" : "var(--text-disabled)",
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
                    background: isActionableSource(source) ? "var(--color-brand-green)" : "var(--text-disabled)",
                  }} />
                </div>
                <div style={{ marginTop: 3, fontSize: 12, color: isActionableSource(source) ? "var(--text-secondary)" : "var(--text-disabled)" }}>
                  {sourceStatusLabel(source)}
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

        {(sourcesQuery.error || searchQuery.error || searchQuery.data?.error_message) && (
          <div style={{ margin: "10px 16px 0", color: "var(--color-badge)", fontSize: 13 }}>
            {humanizeR18Error(
              (sourcesQuery.error instanceof Error ? sourcesQuery.error.message : null) ||
              (searchQuery.error instanceof Error ? searchQuery.error.message : null) ||
              searchQuery.data?.error_message ||
              null,
            )}
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
              {(searchQuery.data?.items ?? []).filter((item) => !isSourceFailureItem(item)).map((item) => (
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

              {(searchQuery.data?.items ?? []).filter((item) => !isSourceFailureItem(item)).length === 0 && !searchQuery.isLoading && (
                <div style={{
                  gridColumn: "1 / -1", padding: 60, textAlign: "center",
                  color: "var(--text-disabled)", fontSize: 14,
                }}>
                  {searchQuery.data?.error_message
                    ? humanizeR18Error(searchQuery.data.error_message)
                    : "暂无结果，请搜索关键词"}
                </div>
              )}
            </div>
          )}
        </GlassScrollArea>
      </section>

      {accountActionError && (
        <div style={{
          position: "fixed", left: 16, right: 16, bottom: 16, zIndex: 96,
          maxWidth: 520, margin: "0 auto",
          padding: "10px 12px", borderRadius: 12,
          border: "1px solid var(--divider)",
          background: "var(--glass-btn-bg)",
          color: "var(--color-badge)", fontSize: 13,
        }}>
          {humanizeR18Error(accountActionError)}
        </div>
      )}

      {accountPanelOpen && (
        <AccountManagerPanel
          accounts={managedAccounts}
          busySourceId={accountBusySourceId}
          drafts={accountDrafts}
          onDraftChange={updateAccountDraft}
          onSave={(sourceId) => { void saveAccount(sourceId) }}
          onLogin={(sourceId) => { void loginAccount(sourceId) }}
          onClear={(sourceId) => { void clearAccount(sourceId) }}
          onClose={() => setAccountPanelOpen(false)}
        />
      )}

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
