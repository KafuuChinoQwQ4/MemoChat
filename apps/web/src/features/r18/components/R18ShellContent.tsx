/** R18ShellContent — source switcher + search + chapter reader */
import { useEffect, useMemo, useRef, useState, type MouseEvent } from "react"
import { useInfiniteQuery, useMutation, useQuery } from "@tanstack/react-query"
import { useSessionStore } from "@/core/session/sessionStore"
import {
  createR18Api,
  type R18ComicItem,
  type R18LibraryFolder,
  type R18LibraryItem,
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
import { defaultSortForSource, filterTagOptions, sourceFilterConfig } from "./r18SourceFilters"
import {
  flattenR18SearchPages,
  hasMoreR18SearchPages,
  latestR18SearchPage,
  shouldLoadMoreR18SearchOnScroll,
} from "./r18SearchPagination"

// ─── Types ────────────────────────────────────────────────────────────────

// ─── Helpers ──────────────────────────────────────────────────────────────

function sourceTitle(source: R18Source): string {
  return source.title?.trim() || source.name?.trim() || source.id
}

function sourceStatusLabel(source: R18Source): string {
  if (source.enabled) {
    if (source.account_status === "authenticated" && source.has_account) return "已登录"
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
  if (message.startsWith("exhentai requires e-hentai login")) {
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
  if (text.startsWith("exhentai requires e-hentai login")) {
    return "ExHentai 需要先登录 E-Hentai（账密 / Cookie / 网页登入），请在「账号管理」中配置"
  }
  if (text.includes("sad panda")) {
    return "ExHentai 访问被拒绝（Sad Panda）：Cookie 无效或账号无 Ex 权限，请重新登录并确认含 igneous"
  }
  if (text.startsWith("e-hentai login failed")) {
    return text.replace("e-hentai login failed:", "E-Hentai 登录失败：")
  }
  if (text.startsWith("Picacg login token missing") || text.startsWith("Picacg login returned success without a session")) {
    return "哔咔上游未返回登录会话，请检查签名配置或稍后重试"
  }
  if (text.startsWith("Picacg login failed")) {
    return text.replace("Picacg login failed:", "哔咔登录失败：")
  }
  if (text.startsWith("JM login failed")) {
    return text
      .replace("JM login failed:", "禁漫登录失败：")
      .replace("JM login failed", "禁漫登录失败")
  }
  if (text.includes("stream truncated") || text.includes("unexpected eof")) {
    return `禁漫上游连接被中断（${text}）。通常是节点 TLS 异常，请重试；若持续失败可能是账号/密码错误或当前网络无法访问 JM API。`
  }
  if (text === "JM username/password required") {
    return "请填写禁漫账号和密码"
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
      className={styles.dialogClose ?? ""}
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
      className={styles.dialogBackdrop ?? ""}
    >
      <GlassSurface
        elevated
        onClick={(e) => e.stopPropagation()}
        className={styles.dialogPanel ?? ""}
      >
        <div className={styles.dialogHeader ?? ""}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div id="r18-chapter-dialog-title" className={styles.dialogTitle ?? ""}>
              {comic.title || "未命名"}
            </div>
            {(comic.author || comic.subtitle) && (
              <div className={styles.dialogSubtitle ?? ""}>
                {comic.author || comic.subtitle}
              </div>
            )}
          </div>
          <CloseButton onClose={onClose} />
        </div>

        <GlassScrollArea className={styles.dialogBody ?? ""}>
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
          <div className={styles.chapterGrid ?? ""}>
            {(chaptersQuery.data ?? []).map((ch) => (
              <button
                key={ch.chapter_id}
                type="button"
                className={styles.chapterChip ?? ""}
                onClick={() => onSelectChapter(ch.chapter_id, ch.title || `第 ${(ch.index ?? 0) + 1} 话`)}
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
    <div className={styles.readerPagePlaceholder ?? ""}>
      <Spinner size={22} />
    </div>
  )
  return (
    <div className={styles.readerPageFrame ?? ""}>
      <img
        src={loaded}
        alt=""
        loading="lazy"
        decoding="async"
        className={styles.readerPageImage ?? ""}
      />
    </div>
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
      className={styles.readerBackdrop ?? ""}
    >
      <div className={styles.readerToolbar ?? ""}>
        <button
          type="button"
          onClick={onClose}
          aria-label="返回"
          className={styles.readerBackButton ?? ""}
        >
          ← 返回
        </button>
        <div id="r18-reader-dialog-title" className={styles.readerTitle ?? ""}>
          {comic.title || "未命名"} · {chapterTitle}
        </div>
      </div>

      <GlassScrollArea className={styles.readerScroll ?? ""}>
        <div className={styles.readerPages ?? ""}>
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

function comicKey(sourceId?: string, comicId?: string): string {
  return `${sourceId ?? ""}::${comicId ?? ""}`
}

function LibraryPanel({
  folders,
  items,
  selectedFolderId,
  busy,
  error,
  newFolderName,
  onNewFolderNameChange,
  onSelectFolder,
  onCreateFolder,
  onDeleteFolder,
  onOpenComic,
  onUnfavorite,
  onClose,
}: {
  folders: R18LibraryFolder[]
  items: R18LibraryItem[]
  selectedFolderId: string
  busy: boolean
  error: string | null
  newFolderName: string
  onNewFolderNameChange: (value: string) => void
  onSelectFolder: (folderId: string) => void
  onCreateFolder: () => void
  onDeleteFolder: (folderId: string) => void
  onOpenComic: (item: R18LibraryItem) => void
  onUnfavorite: (item: R18LibraryItem) => void
  onClose: () => void
}) {
  const dialogRef = useDialogAccessibility(onClose)
  const filtered = selectedFolderId
    ? items.filter((item) => (item.folder_ids ?? []).includes(selectedFolderId))
    : items
  return (
    <div
      ref={dialogRef}
      role="dialog"
      aria-modal="true"
      aria-labelledby="r18-library-dialog-title"
      tabIndex={-1}
      onClick={onClose}
      className={styles.dialogBackdrop ?? ""}
      style={{ zIndex: 92 }}
    >
      <GlassSurface
        elevated
        onClick={(e) => e.stopPropagation()}
        className={`${styles.dialogPanel ?? ""} ${styles.accountDialogPanel ?? ""}`}
        style={{ width: "min(920px, 96vw)", maxHeight: "88vh" }}
      >
        <div className={styles.dialogHeader ?? ""}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div id="r18-library-dialog-title" className={styles.dialogTitle ?? ""}>
              我的收藏
            </div>
            <div className={styles.dialogSubtitle ?? ""}>
              自定义收藏夹（如「全彩」「无码」），把喜欢的本子归档进去
            </div>
          </div>
          <CloseButton onClose={onClose} />
        </div>

        <div style={{ display: "grid", gridTemplateColumns: "220px 1fr", minHeight: 0, flex: 1 }}>
          <div style={{ borderRight: "1px solid var(--divider)", padding: 12, display: "grid", gap: 10, alignContent: "start" }}>
            <GlassButton
              variant={selectedFolderId === "" ? "primary" : "default"}
              onClick={() => onSelectFolder("")}
              style={{ fontSize: 12, justifyContent: "flex-start" }}
            >
              全部收藏 ({items.length})
            </GlassButton>
            {folders.map((folder) => {
              const count = items.filter((item) => (item.folder_ids ?? []).includes(folder.id)).length
              return (
                <div key={folder.id} style={{ display: "grid", gap: 6 }}>
                  <GlassButton
                    variant={selectedFolderId === folder.id ? "primary" : "default"}
                    onClick={() => onSelectFolder(folder.id)}
                    style={{ fontSize: 12, justifyContent: "flex-start" }}
                  >
                    {folder.name} ({count})
                  </GlassButton>
                  {folder.id !== "default" && (
                    <GlassButton
                      disabled={busy}
                      onClick={() => onDeleteFolder(folder.id)}
                      style={{ fontSize: 11, padding: "4px 8px" }}
                    >
                      删除收藏夹
                    </GlassButton>
                  )}
                </div>
              )
            })}
            <div style={{ marginTop: 8, display: "grid", gap: 8 }}>
              <GlassTextField
                value={newFolderName}
                onChange={(e) => onNewFolderNameChange(e.target.value)}
                placeholder="新收藏夹名称，如 全彩"
              />
              <GlassButton
                variant="primary"
                disabled={busy || !newFolderName.trim()}
                onClick={onCreateFolder}
                style={{ fontSize: 12 }}
              >
                新建收藏夹
              </GlassButton>
            </div>
            {error && (
              <div style={{ fontSize: 12, color: "var(--color-badge)" }}>{error}</div>
            )}
          </div>

          <GlassScrollArea style={{ padding: 14, minHeight: 360 }}>
            {filtered.length === 0 ? (
              <div style={{ padding: 40, textAlign: "center", color: "var(--text-secondary)", fontSize: 13 }}>
                该收藏夹还没有本子，去搜索列表点 ♥ 收藏吧
              </div>
            ) : (
              <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(150px, 1fr))", gap: 12 }}>
                {filtered.map((item) => (
                  <GlassSurface
                    key={comicKey(item.source_id, item.comic_id)}
                    elevated
                    style={{ overflow: "hidden" }}
                  >
                    <div
                      role="button"
                      tabIndex={0}
                      style={{ cursor: "pointer" }}
                      onClick={() => onOpenComic(item)}
                      onKeyDown={(e) => {
                        if (e.key === "Enter" || e.key === " ") {
                          e.preventDefault()
                          onOpenComic(item)
                        }
                      }}
                    >
                      <R18Cover
                        {...(item.cover !== undefined ? { cover: item.cover } : {})}
                        {...(item.title !== undefined ? { title: item.title } : {})}
                      />
                      <div style={{ padding: "10px 11px 8px" }}>
                        <div style={{
                          fontWeight: 700, fontSize: 13, lineHeight: 1.35,
                          overflow: "hidden", display: "-webkit-box",
                          WebkitLineClamp: 2, WebkitBoxOrient: "vertical",
                        }}>
                          {item.title || item.comic_id}
                        </div>
                        <div style={{ marginTop: 4, fontSize: 11, color: "var(--text-secondary)" }}>
                          {item.source_id}
                        </div>
                      </div>
                    </div>
                    <div style={{ padding: "0 10px 10px", display: "flex", gap: 6 }}>
                      <GlassButton
                        disabled={busy}
                        onClick={() => onUnfavorite(item)}
                        style={{ fontSize: 11, padding: "4px 8px" }}
                      >
                        取消收藏
                      </GlassButton>
                    </div>
                  </GlassSurface>
                ))}
              </div>
            )}
          </GlassScrollArea>
        </div>
      </GlassSurface>
    </div>
  )
}

function FavoriteFolderPicker({
  comic,
  folders,
  initialFolderIds,
  busy,
  onConfirm,
  onClose,
}: {
  comic: R18ComicItem
  folders: R18LibraryFolder[]
  initialFolderIds: string[]
  busy: boolean
  onConfirm: (folderIds: string[]) => void
  onClose: () => void
}) {
  const dialogRef = useDialogAccessibility(onClose)
  const [selected, setSelected] = useState<string[]>(
    initialFolderIds.length > 0 ? initialFolderIds : ["default"],
  )
  const toggle = (id: string) => {
    setSelected((prev) => {
      if (prev.includes(id)) {
        const next = prev.filter((x) => x !== id)
        return next.length === 0 ? ["default"] : next
      }
      return [...prev, id]
    })
  }
  return (
    <div
      ref={dialogRef}
      role="dialog"
      aria-modal="true"
      aria-labelledby="r18-fav-picker-title"
      tabIndex={-1}
      onClick={onClose}
      className={styles.dialogBackdrop ?? ""}
      style={{ zIndex: 93 }}
    >
      <GlassSurface
        elevated
        onClick={(e) => e.stopPropagation()}
        className={styles.dialogPanel ?? ""}
        style={{ width: "min(420px, 94vw)" }}
      >
        <div className={styles.dialogHeader ?? ""}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div id="r18-fav-picker-title" className={styles.dialogTitle ?? ""}>
              收藏到…
            </div>
            <div className={styles.dialogSubtitle ?? ""}>
              {comic.title || comic.comic_id}
            </div>
          </div>
          <CloseButton onClose={onClose} />
        </div>
        <div style={{ padding: 14, display: "grid", gap: 8 }}>
          {folders.map((folder) => {
            const checked = selected.includes(folder.id)
            return (
              <label
                key={folder.id}
                style={{
                  display: "flex",
                  alignItems: "center",
                  gap: 10,
                  padding: "10px 12px",
                  borderRadius: 12,
                  border: "1px solid var(--divider)",
                  cursor: "pointer",
                  background: checked
                    ? "color-mix(in srgb, var(--color-brand-green, #22c55e) 12%, transparent)"
                    : "transparent",
                }}
              >
                <input
                  type="checkbox"
                  checked={checked}
                  onChange={() => toggle(folder.id)}
                />
                <span style={{ fontSize: 13, fontWeight: 600 }}>{folder.name}</span>
              </label>
            )
          })}
          <div style={{ display: "flex", gap: 8, marginTop: 8 }}>
            <GlassButton
              variant="primary"
              disabled={busy}
              onClick={() => onConfirm(selected)}
              style={{ fontSize: 12 }}
            >
              {busy ? "保存中…" : "确认收藏"}
            </GlassButton>
            <GlassButton disabled={busy} onClick={onClose} style={{ fontSize: 12 }}>
              取消
            </GlassButton>
          </div>
        </div>
      </GlassSurface>
    </div>
  )
}

// ─── Account manager ─────────────────────────────────────────────────────

function accountStatusLabel(account: R18ManagedAccount): string {
  const interaction = accountInteractionKind(account)
  if (interaction === "optional-account") {
    if (account.status === "authenticated" && account.has_session) return "已登录"
    if (account.status === "error") return account.message?.trim() || "登录失败"
    return "可直接访问，也可登录账号"
  }
  if (interaction === "optional-cookie") {
    if (account.status === "authenticated" && account.has_session) return "可选 Cookie 已配置"
    if (account.status === "error") return account.message?.trim() || "配置失败"
    return "可直接访问，也可配置 Cookie"
  }
  if (interaction === "required-ehentai-auth") {
    if (account.status === "authenticated" && account.has_session) return "已登录（绑定 E-Hentai）"
    if (account.status === "error") return account.message?.trim() || "登录失败"
    if (account.status === "configured") return "已保存，待验证 Ex 权限"
    return "必须登录 E-Hentai（账密 / Cookie / 网页）"
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

function isAccountLoggedIn(account: R18ManagedAccount): boolean {
  return account.status === "authenticated" && Boolean(account.has_session)
}

function accountBadgeLabel(account: R18ManagedAccount, interaction: ReturnType<typeof accountInteractionKind>): string {
  if (isAccountLoggedIn(account)) return "已登录"
  if (interaction === "required-account") return "需要账号"
  if (interaction === "required-ehentai-auth") return "需要 E-H 登录"
  if (interaction === "optional-account") return "可选账号"
  if (interaction === "optional-cookie") return "可选 Cookie"
  return "直接访问"
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
      className={styles.dialogBackdrop ?? ""}
      style={{ zIndex: 92 }}
    >
      <GlassSurface
        elevated
        onClick={(e) => e.stopPropagation()}
        className={`${styles.dialogPanel ?? ""} ${styles.accountDialogPanel ?? ""}`}
      >
        <div className={styles.dialogHeader ?? ""}>
          <div style={{ flex: 1, minWidth: 0 }}>
            <div id="r18-account-dialog-title" className={styles.dialogTitle ?? ""}>
              内容源账号管理
            </div>
            <div className={styles.dialogSubtitle ?? ""}>
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
              const requiredEhentaiAuth = interaction === "required-ehentai-auth"
              const supportsCredentials = interaction !== "none"
              const loggedIn = isAccountLoggedIn(account)
              const badgeLabel = accountBadgeLabel(account, interaction)
              return (
                <GlassSurface
                  key={sourceId}
                  style={{
                    padding: 14,
                    borderRadius: 14,
                    border: loggedIn
                      ? "1px solid color-mix(in srgb, var(--color-success, #22c55e) 55%, var(--divider))"
                      : "1px solid var(--divider)",
                    display: "grid",
                    gap: 10,
                  }}
                >
                  <div style={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", gap: 10 }}>
                    <div style={{ minWidth: 0 }}>
                      <div style={{ display: "flex", alignItems: "center", gap: 8, flexWrap: "wrap" }}>
                        <div style={{ fontWeight: 700, fontSize: 14 }}>
                          {account.name || sourceId}
                        </div>
                        {loggedIn && (
                          <span
                            title={account.username ? `账号：${account.username}` : "已登录"}
                            style={{
                              fontSize: 11,
                              fontWeight: 700,
                              padding: "2px 8px",
                              borderRadius: 999,
                              color: "var(--color-success, #16a34a)",
                              background: "color-mix(in srgb, var(--color-success, #22c55e) 16%, transparent)",
                              border: "1px solid color-mix(in srgb, var(--color-success, #22c55e) 40%, transparent)",
                            }}
                          >
                            ✓ 已登录
                          </span>
                        )}
                      </div>
                      <div style={{ marginTop: 3, fontSize: 12, color: "var(--text-secondary)" }}>
                        {accountStatusLabel(account)}
                        {loggedIn && account.username ? ` · ${account.username}` : ""}
                      </div>
                      {account.message && account.status === "error" && (
                        <div style={{ marginTop: 4, fontSize: 12, color: "var(--color-badge)" }}>
                          {humanizeR18Error(account.message)}
                        </div>
                      )}
                      {requiredEhentaiAuth && (
                        <div style={{ marginTop: 6, fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.55 }}>
                          ExHentai 为 E-Hentai 内网/会员源，必须使用同一 E-Hentai 账号。
                          支持三种方式：① 账密登入 ② 粘贴 Cookie（含 ipb_member_id / ipb_pass_hash / igneous）
                          ③ 网页登入后粘贴 Cookie。
                        </div>
                      )}
                    </div>
                    <span style={{
                      flexShrink: 0,
                      fontSize: 11,
                      padding: "3px 8px",
                      borderRadius: 999,
                      border: loggedIn
                        ? "1px solid color-mix(in srgb, var(--color-success, #22c55e) 45%, var(--divider))"
                        : "1px solid var(--divider)",
                      color: loggedIn
                        ? "var(--color-success, #16a34a)"
                        : (needsAccount || requiredEhentaiAuth) ? "var(--text-primary)" : "var(--text-secondary)",
                      background: loggedIn
                        ? "color-mix(in srgb, var(--color-success, #22c55e) 14%, transparent)"
                        : "var(--glass-btn-bg)",
                      fontWeight: loggedIn ? 700 : 400,
                    }}>
                      {badgeLabel}
                    </span>
                  </div>

                  {supportsCredentials ? (
                    <>
                      <GlassTextField
                        value={draft.username}
                        onChange={(e) => onDraftChange(sourceId, "username", e.target.value)}
                        placeholder={
                          optionalCookie
                            ? "备注名（可选）"
                            : requiredEhentaiAuth
                              ? "E-Hentai 账号 / 邮箱（账密登入）"
                              : "账号 / 邮箱"
                        }
                        autoComplete="username"
                      />
                      <GlassTextField
                        type="password"
                        value={draft.password}
                        onChange={(e) => onDraftChange(sourceId, "password", e.target.value)}
                        placeholder={
                          optionalCookie
                            ? "可选 Cookie（ipb_member_id=...; ipb_pass_hash=...）"
                            : requiredEhentaiAuth
                              ? "密码 或 完整 Cookie（ipb_member_id=...; igneous=...）"
                              : (account.has_password ? "密码（留空则保留已保存密码）" : "密码")
                        }
                        autoComplete="current-password"
                      />
                      {requiredEhentaiAuth && (
                        <div style={{ display: "flex", flexWrap: "wrap", gap: 8, alignItems: "center" }}>
                          <a
                            href="https://forums.e-hentai.org/index.php?act=Login"
                            target="_blank"
                            rel="noreferrer noopener"
                            style={{ fontSize: 12, color: "var(--text-primary)", textDecoration: "underline" }}
                          >
                            网页登入 E-Hentai 论坛
                          </a>
                          <a
                            href="https://exhentai.org/"
                            target="_blank"
                            rel="noreferrer noopener"
                            style={{ fontSize: 12, color: "var(--text-secondary)", textDecoration: "underline" }}
                          >
                            打开 ExHentai 校验权限
                          </a>
                          <span style={{ fontSize: 11, color: "var(--text-disabled)" }}>
                            登录后从浏览器复制 Cookie 粘贴到上方密码框
                          </span>
                        </div>
                      )}
                      <div style={{ display: "flex", flexWrap: "wrap", gap: 8 }}>
                        <GlassButton
                          variant="primary"
                          disabled={busy}
                          onClick={() => onSave(sourceId)}
                          style={{ fontSize: 12 }}
                        >
                          {busy ? "处理中…" : requiredEhentaiAuth ? "保存并验证" : "保存并登录"}
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
  const [sort, setSort] = useState("")
  const [tag, setTag] = useState("")
  const [submittedSort, setSubmittedSort] = useState("")
  const [submittedTag, setSubmittedTag] = useState("")
  const [customTag, setCustomTag] = useState("")
  const [filterBarOpen, setFilterBarOpen] = useState(false)
  const [sortSectionOpen, setSortSectionOpen] = useState(false)
  const [tagSectionOpen, setTagSectionOpen] = useState(false)
  const [tagQuery, setTagQuery] = useState("")

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
  const [libraryOpen, setLibraryOpen] = useState(false)
  const [libraryFolderId, setLibraryFolderId] = useState("")
  const [newFolderName, setNewFolderName] = useState("")
  const [libraryError, setLibraryError] = useState<string | null>(null)
  const [libraryBusy, setLibraryBusy] = useState(false)
  const [favoritePickerComic, setFavoritePickerComic] = useState<R18ComicItem | null>(null)
  const [favoriteBusyKey, setFavoriteBusyKey] = useState<string | null>(null)

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

  const libraryQuery = useQuery({
    queryKey: ["r18", "library", uid],
    enabled: authReady && accessQuery.data?.allowed === true,
    queryFn: async () => {
      if (!authReady || accessQuery.data?.allowed !== true) throw new Error("R18 access denied")
      return api.getLibrary()
    },
  })

  const libraryFolders = libraryQuery.data?.folders ?? []
  const libraryItems = libraryQuery.data?.items ?? []
  const favoritedKeys = useMemo(() => {
    const set = new Set<string>()
    for (const item of libraryItems) {
      set.add(comicKey(item.source_id, item.comic_id))
    }
    return set
  }, [libraryItems])
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

  const filterConfig = useMemo(
    () => sourceFilterConfig(selectedSource?.id),
    [selectedSource?.id],
  )

  useEffect(() => {
    const nextSort = defaultSortForSource(selectedSource?.id)
    setSort(nextSort)
    setSubmittedSort(nextSort)
    setTag("")
    setSubmittedTag("")
    setCustomTag("")
    setTagQuery("")
  }, [selectedSource?.id])

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

  const searchQuery = useInfiniteQuery({
    queryKey: ["r18", "search", uid, selectedSource?.id, submittedKeyword, submittedSort, submittedTag],
    enabled: authReady && accessQuery.data?.allowed === true && Boolean(selectedSource?.id),
    initialPageParam: 1,
    queryFn: async ({ pageParam }) => {
      if (!authReady || accessQuery.data?.allowed !== true || !selectedSource?.id) {
        throw new Error("Missing R18 search input")
      }
      // Every source uses the same rule: one scroll load = one upstream page.
      return api.search(selectedSource.id, submittedKeyword, pageParam, {
        sort: submittedSort,
        tag: submittedTag,
      })
    },
    getNextPageParam: (lastPage, _pages, lastPageParam) => {
      const pageItems = (lastPage.items ?? []).filter((item) => !isSourceFailureItem(item))
      if (!hasMoreR18SearchPages(lastPageParam, lastPage.max_page, pageItems.length)) {
        return undefined
      }
      return lastPageParam + 1
    },
  })

  const searchItems = useMemo(
    () => flattenR18SearchPages(searchQuery.data?.pages ?? []).filter((item) => !isSourceFailureItem(item)),
    [searchQuery.data?.pages],
  )
  const latestSearchPage = useMemo(
    () => latestR18SearchPage(searchQuery.data?.pages ?? []),
    [searchQuery.data?.pages],
  )
  const searchScrollRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const area = searchScrollRef.current
    if (!area) return

    const onScroll = () => {
      const distanceToBottom = area.scrollHeight - area.scrollTop - area.clientHeight
      if (
        !shouldLoadMoreR18SearchOnScroll(distanceToBottom, {
          hasMore: Boolean(searchQuery.hasNextPage),
          isLoading: searchQuery.isLoading,
          isFetchingNextPage: searchQuery.isFetchingNextPage,
        })
      ) {
        return
      }
      void searchQuery.fetchNextPage()
    }

    area.addEventListener("scroll", onScroll, { passive: true })
    onScroll()
    return () => area.removeEventListener("scroll", onScroll)
  }, [
    searchItems.length,
    searchQuery.fetchNextPage,
    searchQuery.hasNextPage,
    searchQuery.isFetchingNextPage,
    searchQuery.isLoading,
    selectedSource?.id,
    submittedKeyword,
    submittedSort,
    submittedTag,
  ])

  async function attestR18Access() {
    try {
      await attestMutation.mutateAsync()
    } finally {
      await accessQuery.refetch()
    }
  }

  function submitSearch(next?: { sort?: string; tag?: string }) {
    const nextSort = next?.sort ?? sort
    const nextTag = (next?.tag ?? tag).trim()
    setSort(nextSort)
    setTag(nextTag)
    setSubmittedKeyword(keyword.trim())
    setSubmittedSort(nextSort)
    setSubmittedTag(nextTag)
  }

  function applySort(nextSort: string) {
    submitSearch({ sort: nextSort, tag })
  }

  function applyTag(nextTag: string) {
    setCustomTag(nextTag)
    submitSearch({ sort, tag: nextTag })
  }

  function applyCustomTag() {
    applyTag(customTag.trim())
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

  async function refreshLibrary() {
    await libraryQuery.refetch()
  }

  async function createLibraryFolder() {
    const name = newFolderName.trim()
    if (!name) return
    setLibraryBusy(true)
    setLibraryError(null)
    try {
      const folder = await api.createFolder(name)
      setNewFolderName("")
      setLibraryFolderId(folder.id)
      await refreshLibrary()
    } catch (error) {
      setLibraryError(error instanceof Error ? error.message : "创建收藏夹失败")
    } finally {
      setLibraryBusy(false)
    }
  }

  async function deleteLibraryFolder(folderId: string) {
    setLibraryBusy(true)
    setLibraryError(null)
    try {
      await api.deleteFolder(folderId)
      if (libraryFolderId === folderId) setLibraryFolderId("")
      await refreshLibrary()
    } catch (error) {
      setLibraryError(error instanceof Error ? error.message : "删除收藏夹失败")
    } finally {
      setLibraryBusy(false)
    }
  }

  async function confirmFavorite(comic: R18ComicItem, folderIds: string[]) {
    const sourceId = comic.source_id || selectedSource?.id || ""
    const comicId = comic.comic_id || ""
    if (!sourceId || !comicId) return
    const key = comicKey(sourceId, comicId)
    setFavoriteBusyKey(key)
    setLibraryError(null)
    try {
      await api.toggleFavorite({
        sourceId,
        comicId,
        favorited: true,
        title: comic.title ?? "",
        cover: comic.cover ?? "",
        author: comic.author ?? "",
        subtitle: comic.subtitle ?? "",
        folderIds,
      })
      setFavoritePickerComic(null)
      await refreshLibrary()
    } catch (error) {
      setLibraryError(error instanceof Error ? error.message : "收藏失败")
    } finally {
      setFavoriteBusyKey(null)
    }
  }

  async function unfavoriteItem(item: { source_id?: string; comic_id?: string }) {
    const sourceId = item.source_id || ""
    const comicId = item.comic_id || ""
    if (!sourceId || !comicId) return
    const key = comicKey(sourceId, comicId)
    setFavoriteBusyKey(key)
    setLibraryError(null)
    try {
      await api.toggleFavorite({
        sourceId,
        comicId,
        favorited: false,
      })
      await refreshLibrary()
    } catch (error) {
      setLibraryError(error instanceof Error ? error.message : "取消收藏失败")
    } finally {
      setFavoriteBusyKey(null)
    }
  }

  function onFavoriteClick(item: R18ComicItem, e: MouseEvent) {
    e.stopPropagation()
    const sourceId = item.source_id || selectedSource?.id || ""
    const comicId = item.comic_id || ""
    const key = comicKey(sourceId, comicId)
    if (favoritedKeys.has(key)) {
      void unfavoriteItem({ source_id: sourceId, comic_id: comicId })
      return
    }
    setFavoritePickerComic({
      ...item,
      source_id: sourceId,
      comic_id: comicId,
    })
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
            <div style={{ display: "flex", gap: 6, flexShrink: 0 }}>
              <GlassButton
                onClick={() => {
                  setLibraryError(null)
                  setLibraryOpen(true)
                  void libraryQuery.refetch()
                }}
                style={{ padding: "5px 10px", fontSize: 12 }}
              >
                收藏
              </GlassButton>
              <GlassButton
                onClick={() => {
                  setAccountActionError(null)
                  setAccountPanelOpen(true)
                }}
                style={{ padding: "5px 10px", fontSize: 12 }}
              >
                账号
              </GlassButton>
            </div>
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
                  <div style={{ display: "flex", alignItems: "center", gap: 6, flexShrink: 0 }}>
                    {source.account_status === "authenticated" && (
                      <span style={{
                        fontSize: 10,
                        fontWeight: 700,
                        padding: "1px 6px",
                        borderRadius: 999,
                        color: "var(--color-success, #16a34a)",
                        background: "color-mix(in srgb, var(--color-success, #22c55e) 14%, transparent)",
                        border: "1px solid color-mix(in srgb, var(--color-success, #22c55e) 35%, transparent)",
                      }}>
                        已登录
                      </span>
                    )}
                    <span style={{
                      width: 8, height: 8, borderRadius: "50%",
                      background: isActionableSource(source) ? "var(--color-brand-green)" : "var(--text-disabled)",
                    }} />
                  </div>
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
          <GlassButton onClick={() => submitSearch()} variant="primary" style={{ flexShrink: 0 }}>搜索</GlassButton>
        </div>

        {/* Source-native sort / tag filters (whole bar collapsible) */}
        <div className={styles.filterBar}>
          <button
            type="button"
            className={styles.filterMasterToggle}
            aria-expanded={filterBarOpen}
            onClick={() => setFilterBarOpen((open) => !open)}
          >
            <span className={styles.filterMasterLeft}>
              <span className={styles.filterLabel}>筛选</span>
              <span className={styles.filterSummaryInline}>
                {submittedKeyword ? `关键词「${submittedKeyword}」` : "无关键词"}
                {` · ${
                  filterConfig.sorts.find((s) => s.id === submittedSort)?.label
                  ?? filterConfig.sorts[0]?.label
                  ?? "默认排序"
                }`}
                {` · ${
                  submittedTag
                    ? (filterConfig.tags.find((t) => t.id === submittedTag)?.label ?? submittedTag)
                    : "全部 tag"
                }`}
              </span>
            </span>
            <span className={styles.filterToggleMeta}>
              <span className={styles.filterCount}>{Math.max(0, filterConfig.tags.length - 1)}</span>
              <span className={styles.filterChevron} data-open={filterBarOpen ? "true" : "false"}>▾</span>
            </span>
          </button>

          {filterBarOpen && (
            <div className={styles.filterBarBody}>
              <div className={styles.filterGroup}>
                <button
                  type="button"
                  className={styles.filterToggle}
                  aria-expanded={sortSectionOpen}
                  onClick={() => setSortSectionOpen((open) => !open)}
                >
                  <span className={styles.filterLabel}>排序 / 分类</span>
                  <span className={styles.filterToggleMeta}>
                    {filterConfig.sorts.find((s) => s.id === submittedSort)?.label
                      ?? filterConfig.sorts[0]?.label
                      ?? "默认"}
                    <span className={styles.filterChevron} data-open={sortSectionOpen ? "true" : "false"}>▾</span>
                  </span>
                </button>
                {sortSectionOpen && (
                  <div className={styles.filterPanel}>
                    <div className={styles.filterChips}>
                      {filterConfig.sorts.map((option) => {
                        const active = submittedSort === option.id
                        return (
                          <button
                            key={`sort-${option.id || "default"}`}
                            type="button"
                            className={styles.filterChip}
                            data-active={active ? "true" : "false"}
                            onClick={() => applySort(option.id)}
                          >
                            {option.label}
                          </button>
                        )
                      })}
                    </div>
                  </div>
                )}
              </div>

              <div className={styles.filterGroup}>
                <button
                  type="button"
                  className={styles.filterToggle}
                  aria-expanded={tagSectionOpen}
                  onClick={() => setTagSectionOpen((open) => !open)}
                >
                  <span className={styles.filterLabel}>Tag / 分类</span>
                  <span className={styles.filterToggleMeta}>
                    {submittedTag
                      ? (filterConfig.tags.find((t) => t.id === submittedTag)?.label ?? submittedTag)
                      : "全部"}
                    <span className={styles.filterCount}>{Math.max(0, filterConfig.tags.length - 1)}</span>
                    <span className={styles.filterChevron} data-open={tagSectionOpen ? "true" : "false"}>▾</span>
                  </span>
                </button>
                {tagSectionOpen && (
                  <div className={styles.filterPanel}>
                    <div className={styles.tagSearchRow}>
                      <GlassTextField
                        value={tagQuery}
                        onChange={(e) => setTagQuery(e.target.value)}
                        placeholder="搜索官方 tag…"
                      />
                      {tagQuery ? (
                        <GlassButton
                          onClick={() => setTagQuery("")}
                          style={{ flexShrink: 0, padding: "5px 12px", fontSize: 12 }}
                        >
                          清空
                        </GlassButton>
                      ) : null}
                    </div>
                    <div className={styles.filterChipsScroll}>
                      <div className={styles.filterChips}>
                        {filterTagOptions(filterConfig.tags, tagQuery).map((option) => {
                          const active = submittedTag === option.id
                          return (
                            <button
                              key={`tag-${option.id || "all"}`}
                              type="button"
                              className={styles.filterChip}
                              data-active={active ? "true" : "false"}
                              onClick={() => applyTag(option.id)}
                            >
                              {option.label}
                            </button>
                          )
                        })}
                      </div>
                      {filterTagOptions(filterConfig.tags, tagQuery).length === 0 && (
                        <div className={styles.filterEmpty}>没有匹配的 tag，可改关键词或用下方自定义</div>
                      )}
                    </div>
                    {filterConfig.allowCustomTag && (
                      <div className={styles.customTagRow}>
                        <GlassTextField
                          value={customTag}
                          onChange={(e) => setCustomTag(e.target.value)}
                          placeholder={filterConfig.tagPlaceholder}
                          onKeyDown={(e) => {
                            if (e.key === "Enter") {
                              e.preventDefault()
                              applyCustomTag()
                            }
                          }}
                        />
                        <GlassButton
                          onClick={applyCustomTag}
                          style={{ flexShrink: 0, padding: "5px 12px", fontSize: 12 }}
                        >
                          应用
                        </GlassButton>
                        {submittedTag ? (
                          <GlassButton
                            onClick={() => applyTag("")}
                            style={{ flexShrink: 0, padding: "5px 12px", fontSize: 12 }}
                          >
                            清除
                          </GlassButton>
                        ) : null}
                      </div>
                    )}
                  </div>
                )}
              </div>
            </div>
          )}
        </div>


        {(sourcesQuery.error || searchQuery.error || latestSearchPage?.error_message) && (
          <div style={{ margin: "10px 16px 0", color: "var(--color-badge)", fontSize: 13 }}>
            {humanizeR18Error(
              (sourcesQuery.error instanceof Error ? sourcesQuery.error.message : null) ||
              (searchQuery.error instanceof Error ? searchQuery.error.message : null) ||
              latestSearchPage?.error_message ||
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
        <GlassScrollArea ref={searchScrollRef} style={{ flex: 1, minHeight: 0, padding: 16 }}>
          {searchQuery.isLoading ? (
            <div style={{ display: "grid", placeItems: "center", height: 200 }}>
              <Spinner size={28} />
            </div>
          ) : (
            <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(170px, 1fr))", gap: 12 }}>
              {searchItems.map((item) => {
                const key = comicKey(item.source_id || selectedSource?.id, item.comic_id)
                const isFav = favoritedKeys.has(key)
                const favBusy = favoriteBusyKey === key
                return (
                <GlassSurface
                  key={key}
                  elevated
                  style={{
                    overflow: "hidden", cursor: "pointer",
                    transition: "transform 150ms ease, box-shadow 150ms ease",
                    position: "relative",
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
                  <button
                    type="button"
                    aria-label={isFav ? "取消收藏" : "收藏"}
                    disabled={favBusy || !item.comic_id}
                    onClick={(e) => onFavoriteClick(item, e)}
                    style={{
                      position: "absolute",
                      top: 8,
                      right: 8,
                      zIndex: 2,
                      width: 32,
                      height: 32,
                      borderRadius: 999,
                      border: "1px solid var(--divider)",
                      background: isFav
                        ? "color-mix(in srgb, #ef4444 88%, white)"
                        : "color-mix(in srgb, var(--glass-btn-bg) 88%, transparent)",
                      color: isFav ? "#fff" : "var(--text-primary)",
                      cursor: favBusy ? "wait" : "pointer",
                      fontSize: 15,
                      lineHeight: 1,
                      display: "grid",
                      placeItems: "center",
                      boxShadow: "0 2px 8px rgba(0,0,0,0.18)",
                    }}
                  >
                    {isFav ? "♥" : "♡"}
                  </button>
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
                )
              })}

              {searchItems.length === 0 && !searchQuery.isLoading && (
                <div style={{
                  gridColumn: "1 / -1", padding: 60, textAlign: "center",
                  color: "var(--text-disabled)", fontSize: 14,
                }}>
                  {latestSearchPage?.error_message
                    ? humanizeR18Error(latestSearchPage.error_message)
                    : "暂无结果，可切换排序 / tag 或搜索关键词"}
                </div>
              )}

              {searchItems.length > 0 && (
                <div style={{
                  gridColumn: "1 / -1",
                  padding: "10px 4px 4px",
                  textAlign: "center",
                  color: "var(--text-secondary)",
                  fontSize: 12,
                }}>
                  {searchQuery.isFetchingNextPage
                    ? "正在加载更多…"
                    : searchQuery.hasNextPage
                      ? "继续下滑加载更多"
                      : "已经到底了"}
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

      {libraryOpen && (
        <LibraryPanel
          folders={libraryFolders}
          items={libraryItems}
          selectedFolderId={libraryFolderId}
          busy={libraryBusy}
          error={libraryError}
          newFolderName={newFolderName}
          onNewFolderNameChange={setNewFolderName}
          onSelectFolder={setLibraryFolderId}
          onCreateFolder={() => { void createLibraryFolder() }}
          onDeleteFolder={(folderId) => { void deleteLibraryFolder(folderId) }}
          onOpenComic={(item) => {
            setLibraryOpen(false)
            const comic: R18ComicItem = {
              source_id: item.source_id,
              comic_id: item.comic_id,
            }
            if (item.title) comic.title = item.title
            if (item.cover) comic.cover = item.cover
            if (item.author) comic.author = item.author
            if (item.subtitle) comic.subtitle = item.subtitle
            setComicForChapters(comic)
          }}
          onUnfavorite={(item) => { void unfavoriteItem(item) }}
          onClose={() => setLibraryOpen(false)}
        />
      )}

      {favoritePickerComic && (
        <FavoriteFolderPicker
          comic={favoritePickerComic}
          folders={libraryFolders.length > 0 ? libraryFolders : [{ id: "default", name: "默认收藏" }]}
          initialFolderIds={["default"]}
          busy={favoriteBusyKey === comicKey(favoritePickerComic.source_id, favoritePickerComic.comic_id)}
          onConfirm={(folderIds) => { void confirmFavorite(favoritePickerComic, folderIds) }}
          onClose={() => setFavoritePickerComic(null)}
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
