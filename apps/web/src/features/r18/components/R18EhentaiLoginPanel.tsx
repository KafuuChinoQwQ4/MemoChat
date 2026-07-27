/**
 * R18EhentaiLoginPanel — E-Hentai / ExHentai 三种登录模式
 *
 *  1. 账密登录  — 用户名 + 密码，由服务端向 E-Hentai 论坛发起登录
 *  2. 网页登录  — 打开官方论坛登录页，完成后粘贴 Cookie（浏览器扩展已安装时可自动捕获）
 *  3. 粘贴 Cookie — 直接粘贴含 ipb_member_id / ipb_pass_hash / igneous 的 Cookie 字符串
 *
 * 设计原则：
 *  - "网页登录"无需浏览器扩展；扩展仅作为可选的自动化增强
 *  - 密码值在一次提交后由调用方清空，不在面板内部缓存
 *  - Cookie 字段提交后立即清空，不保留在 React state 中
 */
import { useCallback, useEffect, useRef, useState } from "react"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { R18PasswordField } from "./R18PasswordField"

// ─── Local alias so existing internal callers keep working ───────────────────
const PasswordField = R18PasswordField

// ─── Extension bridge (optional enhancement) ─────────────────────────────────

type ExtensionStatus = { kind: "unknown" } | { kind: "present"; version: number } | { kind: "absent" }

function detectExtension(timeoutMs = 600): Promise<ExtensionStatus> {
  return new Promise((resolve) => {
    const timer = setTimeout(() => {
      window.removeEventListener("message", handler)
      resolve({ kind: "absent" })
    }, timeoutMs)
    function handler(event: MessageEvent) {
      if (event.source !== window) return
      if (event.data?.source !== "memochat-ehentai-extension") return
      if (event.data?.type !== "EHENTAI_BRIDGE_READY") return
      clearTimeout(timer)
      window.removeEventListener("message", handler)
      resolve({ kind: "present", version: event.data.version ?? 1 })
    }
    window.addEventListener("message", handler)
    window.postMessage({ source: "memochat-web", type: "EHENTAI_BRIDGE_CHECK" }, "*")
  })
}

function sendExtensionStart(importId: string, ticket: string, expiresAt: number): Promise<boolean> {
  return new Promise((resolve) => {
    const timer = setTimeout(() => { window.removeEventListener("message", handler); resolve(false) }, 3000)
    function handler(event: MessageEvent) {
      if (event.source !== window) return
      if (event.data?.source !== "memochat-ehentai-extension") return
      if (event.data?.type !== "EHENTAI_START_RESPONSE") return
      clearTimeout(timer)
      window.removeEventListener("message", handler)
      resolve(event.data.success === true)
    }
    window.addEventListener("message", handler)
    window.postMessage({ source: "memochat-web", type: "EHENTAI_START_IMPORT", importId, ticket, expiresAt }, "*")
  })
}

function cancelExtension() {
  window.postMessage({ source: "memochat-web", type: "EHENTAI_CANCEL" }, "*")
}

// ─── Types ───────────────────────────────────────────────────────────────────

type LoginMode = "password" | "web" | "cookie"

export interface EhentaiLoginPanelProps {
  sourceId: string
  optional?: boolean
  loggedIn: boolean
  loggedInUsername?: string
  busy: boolean
  draft: { username: string; password: string }
  hasPassword?: boolean
  hasSession?: boolean
  onDraftChange: (field: "username" | "password", value: string) => void
  onSave: () => void
  onLogin: () => void
  onClear: () => void
  onStartBrowserImport: (sourceId: string) => Promise<{ importId: string; ticket: string; expiresAt: number }>
  onPollImportStatus: (importId: string) => Promise<{ status: string; message?: string }>
  onImportCookiePaste: (sourceId: string, cookieStr: string) => Promise<void>
}

// ─── Tab ─────────────────────────────────────────────────────────────────────

function ModeTab({ label, active, onClick }: { label: string; active: boolean; onClick: () => void }) {
  return (
    <button
      onClick={onClick}
      style={{
        fontSize: 12, padding: "4px 12px", borderRadius: 999, cursor: "pointer",
        border: active ? "1.5px solid var(--color-primary, #6366f1)" : "1px solid var(--divider)",
        background: active ? "color-mix(in srgb, var(--color-primary, #6366f1) 14%, transparent)" : "transparent",
        color: active ? "var(--color-primary, #6366f1)" : "var(--text-secondary)",
        fontWeight: active ? 700 : 400,
      }}
    >
      {label}
    </button>
  )
}

// ─── Component ───────────────────────────────────────────────────────────────

export function R18EhentaiLoginPanel({
  sourceId,
  optional = false,
  loggedIn,
  busy,
  draft,
  hasPassword,
  hasSession,
  onDraftChange,
  onSave,
  onLogin,
  onClear,
  onStartBrowserImport,
  onPollImportStatus,
  onImportCookiePaste,
}: EhentaiLoginPanelProps) {
  const [mode, setMode] = useState<LoginMode>("web")

  // Web-login state
  const [webStep, setWebStep] = useState<"idle" | "opened" | "done" | "failed">("idle")
  const [webCookies, setWebCookies] = useState("")
  const [webCookieError, setWebCookieError] = useState("")

  // Extension state (optional enhancement)
  const [extension, setExtension] = useState<ExtensionStatus>({ kind: "unknown" })
  const [extPhase, setExtPhase] = useState<"idle" | "starting" | "waiting" | "validating" | "done" | "failed">("idle")
  const [extMessage, setExtMessage] = useState("")
  const importIdRef = useRef<string | null>(null)
  const pollRef = useRef<ReturnType<typeof setInterval> | null>(null)

  // Cookie-fields state (individual named fields replace the old paste box)
  const [cookieFields, setCookieFields] = useState<Record<string, string>>({
    ipb_member_id: "", ipb_pass_hash: "", igneous: "", sk: "",
  })
  const [pasteError, setPasteError] = useState("")

  useEffect(() => {
    let alive = true
    detectExtension().then((r) => { if (alive) setExtension(r) })
    return () => { alive = false }
  }, [])

  useEffect(() => () => { if (pollRef.current) clearInterval(pollRef.current) }, [])

  const stopPoll = useCallback(() => { if (pollRef.current) { clearInterval(pollRef.current); pollRef.current = null } }, [])

  const resetAll = useCallback(() => {
    stopPoll()
    setWebStep("idle"); setWebCookies(""); setWebCookieError("")
    setExtPhase("idle"); setExtMessage(""); importIdRef.current = null
    setCookieFields({ ipb_member_id: "", ipb_pass_hash: "", igneous: "", sk: "" })
    setPasteError("")
  }, [stopPoll])

  // ── Web login: open official page then paste ──────────────────────────────

  const openLoginPage = useCallback(() => {
    window.open("https://forums.e-hentai.org/index.php?act=Login", "_blank", "noopener,noreferrer")
    setWebStep("opened")
    setWebCookies("")
    setWebCookieError("")
  }, [])

  const submitWebCookies = useCallback(async () => {
    const raw = webCookies.trim()
    if (!raw) { setWebCookieError("请粘贴从浏览器复制的 Cookie"); return }
    setWebCookieError("")
    try {
      await onImportCookiePaste(sourceId, raw)
      setWebCookies("")
      setWebStep("done")
    } catch (err) {
      setWebCookieError(err instanceof Error ? err.message : "导入失败")
    }
  }, [webCookies, sourceId, onImportCookiePaste])

  // ── Extension flow (optional, only when extension detected) ───────────────

  const startExtensionFlow = useCallback(async () => {
    setExtPhase("starting"); setExtMessage("正在准备票据…")
    try {
      const { importId, ticket, expiresAt } = await onStartBrowserImport(sourceId)
      importIdRef.current = importId
      setExtPhase("starting"); setExtMessage("正在启动扩展…")
      const started = await sendExtensionStart(importId, ticket, expiresAt)
      if (!started) { setExtPhase("failed"); setExtMessage("扩展未响应，请使用下方手动粘贴"); return }
      setExtPhase("waiting"); setExtMessage("请在打开的标签页完成登录…")
      stopPoll()
      pollRef.current = setInterval(async () => {
        const id = importIdRef.current; if (!id) { stopPoll(); return }
        try {
          const { status, message } = await onPollImportStatus(id)
          if (status === "authenticated") { stopPoll(); setExtPhase("done"); setExtMessage(message || "登录成功！") }
          else if (status === "failed")   { stopPoll(); setExtPhase("failed"); setExtMessage(message || "登录失败") }
          else if (status === "expired")  { stopPoll(); setExtPhase("failed"); setExtMessage("票据过期，请重试") }
        } catch { /* transient */ }
      }, 2000)
    } catch (err) {
      setExtPhase("failed"); setExtMessage(err instanceof Error ? err.message : "启动失败")
    }
  }, [sourceId, onStartBrowserImport, onPollImportStatus, stopPoll])

  const cancelExt = useCallback(() => {
    stopPoll(); cancelExtension(); importIdRef.current = null; setExtPhase("idle"); setExtMessage("")
  }, [stopPoll])

  // ── Cookie fields: assemble header and submit ─────────────────────────────

  const submitCookieFields = useCallback(async () => {
    const memberId = (cookieFields.ipb_member_id ?? "").trim()
    const passHash  = (cookieFields.ipb_pass_hash ?? "").trim()
    if (!memberId || !passHash) {
      setPasteError("ipb_member_id 和 ipb_pass_hash 为必填项")
      return
    }
    setPasteError("")
    const parts: string[] = [`ipb_member_id=${memberId}`, `ipb_pass_hash=${passHash}`]
    const igneous = (cookieFields.igneous ?? "").trim()
    const sk      = (cookieFields.sk ?? "").trim()
    if (igneous) parts.push(`igneous=${igneous}`)
    if (sk)      parts.push(`sk=${sk}`)
    try {
      await onImportCookiePaste(sourceId, parts.join("; "))
      setCookieFields({ ipb_member_id: "", ipb_pass_hash: "", igneous: "", sk: "" })
    } catch (err) {
      setPasteError(err instanceof Error ? err.message : "导入失败")
    }
  }, [cookieFields, sourceId, onImportCookiePaste])

  // ── Cookie paste (legacy helper — kept for web-login step 2) ─────────────

  // ─── Render ───────────────────────────────────────────────────────────────

  const extBusy = extPhase === "starting" || extPhase === "waiting" || extPhase === "validating"

  return (
    <div style={{ display: "grid", gap: 10 }}>
      {/* Description */}
      <div style={{ fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.55 }}>
        {optional
          ? "支持三种方式：① 账密登入 ② 网页跳转登入 ③ 粘贴 Cookie"
          : "ExHentai 为 E-Hentai 内网/会员源，必须使用同一 E-Hentai 账号。支持三种方式：① 账密登入 ② 网页跳转登入 ③ 粘贴 Cookie"}
      </div>

      {/* Mode tabs */}
      <div style={{ display: "flex", gap: 6, flexWrap: "wrap" }}>
        <ModeTab label="账密登录"    active={mode === "password"} onClick={() => { setMode("password"); resetAll() }} />
        <ModeTab label="网页登录"    active={mode === "web"}      onClick={() => { setMode("web");      resetAll() }} />
        <ModeTab label="粘贴 Cookie" active={mode === "cookie"}   onClick={() => { setMode("cookie");   resetAll() }} />
      </div>

      {/* ── Mode 1: 账密 ── */}
      {mode === "password" && (
        <div style={{ display: "grid", gap: 8 }}>
          <GlassTextField
            value={draft.username}
            onChange={(e) => onDraftChange("username", e.target.value)}
            placeholder="E-Hentai 账号 / 邮箱"
            autoComplete="username"
          />
          <PasswordField
            value={draft.password}
            onChange={(e) => onDraftChange("password", e.target.value)}
            placeholder={hasPassword ? "密码（留空保留已保存密码）" : "密码"}
            autoComplete="current-password"
          />
          <div style={{ display: "flex", flexWrap: "wrap", gap: 8 }}>
            <GlassButton variant="primary" disabled={busy} onClick={onSave} style={{ fontSize: 12 }}>
              {busy ? "处理中…" : "保存并登录"}
            </GlassButton>
            {(hasPassword || hasSession) && (
              <GlassButton disabled={busy} onClick={onLogin} style={{ fontSize: 12 }}>重新登录</GlassButton>
            )}
            {(hasPassword || hasSession || loggedIn) && (
              <GlassButton disabled={busy} onClick={onClear} style={{ fontSize: 12 }}>清除</GlassButton>
            )}
          </div>
          <div style={{ fontSize: 11, color: "var(--text-disabled)", lineHeight: 1.5 }}>
            服务端会向 E-Hentai 论坛发起登录请求。若遇到 Cloudflare 挑战，请改用「网页登录」。
          </div>
        </div>
      )}

      {/* ── Mode 2: 网页登录（打开官方页 → 完成登录 → 粘贴 cookie） ── */}
      {mode === "web" && (
        <div style={{ display: "grid", gap: 10 }}>

          {/* Step 1: open login page */}
          {webStep === "idle" && (
            <div style={{ display: "grid", gap: 8 }}>
              <div style={{ fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.6 }}>
                点击下方按钮，将在新标签页打开 E-Hentai 官方论坛登录页。完成登录后回到此页，按提示粘贴 Cookie。
              </div>
              <div>
                <GlassButton variant="primary" disabled={busy} onClick={openLoginPage} style={{ fontSize: 12 }}>
                  打开官方登录页
                </GlassButton>
              </div>

              {/* Optional: extension auto-capture */}
              {extension.kind === "present" && (
                <div style={{
                  padding: "8px 12px", borderRadius: 8, fontSize: 12, lineHeight: 1.6,
                  background: "color-mix(in srgb, var(--color-primary, #6366f1) 8%, transparent)",
                  border: "1px solid color-mix(in srgb, var(--color-primary, #6366f1) 25%, transparent)",
                }}>
                  <div style={{ fontWeight: 600, marginBottom: 4 }}>已检测到 MemoChat 扩展</div>
                  <div style={{ color: "var(--text-secondary)", marginBottom: 8 }}>
                    可使用扩展自动捕获 Cookie，无需手动复制粘贴。
                  </div>
                  <GlassButton
                    variant="primary"
                    disabled={busy || extBusy}
                    onClick={startExtensionFlow}
                    style={{ fontSize: 12 }}
                  >
                    使用扩展自动登录
                  </GlassButton>
                </div>
              )}
            </div>
          )}

          {/* Step 2: login page opened, waiting for user to paste */}
          {webStep === "opened" && (
            <div style={{ display: "grid", gap: 8 }}>
              <div style={{
                padding: "10px 12px", borderRadius: 8, fontSize: 12, lineHeight: 1.7,
                background: "color-mix(in srgb, var(--color-primary, #6366f1) 8%, transparent)",
                border: "1px solid color-mix(in srgb, var(--color-primary, #6366f1) 25%, transparent)",
              }}>
                <div style={{ fontWeight: 600, marginBottom: 6 }}>✓ 已打开 E-Hentai 官方登录页</div>
                <div style={{ color: "var(--text-secondary)" }}>
                  请在新标签页完成登录，然后按以下步骤获取 Cookie：
                </div>
                <ol style={{ margin: "6px 0 0 0", paddingLeft: 18, color: "var(--text-secondary)" }}>
                  <li>登录后打开浏览器开发者工具（按 <code>F12</code>）</li>
                  <li>选择 <code>Application</code>（Chrome/Edge）或 <code>Storage</code>（Firefox）标签</li>
                  <li>展开 <code>Cookies → https://e-hentai.org</code></li>
                  <li>复制 <code>ipb_member_id</code> 和 <code>ipb_pass_hash</code> 的值（ExHentai 还需要 <code>igneous</code>）</li>
                </ol>
              </div>

              <div style={{ fontSize: 12, color: "var(--text-secondary)", fontWeight: 600 }}>
                将 Cookie 粘贴到下方（格式：<code style={{ fontSize: 11 }}>ipb_member_id=…; ipb_pass_hash=…</code>）
              </div>

              <PasswordField
                value={webCookies}
                onChange={(e) => { setWebCookies(e.target.value); setWebCookieError("") }}
                placeholder="ipb_member_id=…; ipb_pass_hash=…; igneous=…"
                autoComplete="off"
              />

              {webCookieError && (
                <div style={{ fontSize: 12, color: "var(--color-danger, #ef4444)" }}>{webCookieError}</div>
              )}

              <div style={{ display: "flex", flexWrap: "wrap", gap: 8 }}>
                <GlassButton
                  variant="primary"
                  disabled={busy || !webCookies.trim()}
                  onClick={submitWebCookies}
                  style={{ fontSize: 12 }}
                >
                  {busy ? "导入中…" : "确认并导入"}
                </GlassButton>
                <GlassButton disabled={busy} onClick={openLoginPage} style={{ fontSize: 12 }}>
                  重新打开登录页
                </GlassButton>
                <GlassButton disabled={busy} onClick={() => setWebStep("idle")} style={{ fontSize: 12 }}>
                  取消
                </GlassButton>
              </div>

              <div style={{ fontSize: 11, color: "var(--text-disabled)" }}>
                Cookie 提交后立即清空，不会保留在页面中。
              </div>
            </div>
          )}

          {/* Step 3: done */}
          {webStep === "done" && (
            <div style={{ display: "grid", gap: 8 }}>
              <div style={{
                padding: "10px 12px", borderRadius: 8, fontSize: 12,
                background: "color-mix(in srgb, var(--color-success, #22c55e) 12%, transparent)",
                border: "1px solid color-mix(in srgb, var(--color-success, #22c55e) 35%, transparent)",
                color: "var(--color-success, #16a34a)",
              }}>
                ✓ Cookie 已成功导入
              </div>
              <div style={{ display: "flex", gap: 8 }}>
                <GlassButton disabled={busy} onClick={onClear} style={{ fontSize: 12 }}>清除登录</GlassButton>
                <GlassButton disabled={busy} onClick={() => setWebStep("idle")} style={{ fontSize: 12 }}>重新登录</GlassButton>
              </div>
            </div>
          )}

          {/* Extension phase indicator */}
          {extension.kind === "present" && extPhase !== "idle" && (
            <div style={{ display: "grid", gap: 8 }}>
              <div style={{
                padding: "10px 12px", borderRadius: 8, fontSize: 12,
                background: extPhase === "done"   ? "color-mix(in srgb, var(--color-success, #22c55e) 12%, transparent)"
                           : extPhase === "failed" ? "color-mix(in srgb, var(--color-danger,  #ef4444) 10%, transparent)"
                           : "color-mix(in srgb, var(--color-primary, #6366f1) 10%, transparent)",
                border: `1px solid ${
                  extPhase === "done"   ? "color-mix(in srgb, var(--color-success, #22c55e) 35%, transparent)"
                : extPhase === "failed" ? "color-mix(in srgb, var(--color-danger,  #ef4444) 30%, transparent)"
                : "color-mix(in srgb, var(--color-primary, #6366f1) 30%, transparent)"}`,
              }}>
                {extMessage || extPhase}
              </div>
              {extBusy && (
                <GlassButton disabled={busy} onClick={cancelExt} style={{ fontSize: 12 }}>取消扩展流程</GlassButton>
              )}
            </div>
          )}
        </div>
      )}

      {/* ── Mode 3: 粘贴 Cookie（逐字段输入）── */}
      {mode === "cookie" && (
        <div style={{ display: "grid", gap: 10 }}>
          <div style={{ fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.6 }}>
            在浏览器中登录 E-Hentai 后，按 <code>F12</code> → Application → Cookies → <code>https://e-hentai.org</code>，把各字段的 <strong>value</strong> 粘贴到对应输入框。
          </div>

          {/* Cookie fields with pre-filled names */}
          {(
            [
              { name: "ipb_member_id", required: true,  hint: "数字会员 ID" },
              { name: "ipb_pass_hash", required: true,  hint: "32位 hash" },
              { name: "igneous",       required: false, hint: "ExHentai 专用，无则留空" },
              { name: "sk",            required: false, hint: "可选" },
            ] as const
          ).map(({ name, hint }) => (
            <div key={name} style={{ display: "flex", alignItems: "center", gap: 8 }}>
              <div style={{
                flexShrink: 0,
                minWidth: 140,
                fontSize: 12,
                fontFamily: "monospace",
                fontWeight: 600,
                padding: "6px 10px",
                borderRadius: 6,
                background: "var(--tint-hover)",
                border: "1px solid var(--divider)",
                color: "var(--text-primary)",
                userSelect: "all" as const,
              }}>
                {name}
              </div>
              <span style={{ color: "var(--text-disabled)", fontSize: 14 }}>=</span>
              <div style={{ flex: 1 }}>
                <PasswordField
                  value={cookieFields[name] ?? ""}
                  onChange={(e) => {
                    const val = e.target.value
                    setCookieFields((prev) => ({ ...prev, [name]: val }))
                    setPasteError("")
                  }}
                  placeholder={hint}
                  autoComplete="off"
                />
              </div>
            </div>
          ))}

          {pasteError && (
            <div style={{ fontSize: 12, color: "var(--color-danger, #ef4444)" }}>{pasteError}</div>
          )}
          <div style={{ display: "flex", gap: 8, flexWrap: "wrap" }}>
            <GlassButton
              variant="primary"
              disabled={busy || (!cookieFields.ipb_member_id?.trim() && !cookieFields.ipb_pass_hash?.trim())}
              onClick={submitCookieFields}
              style={{ fontSize: 12 }}
            >
              {busy ? "导入中…" : "导入 Cookie"}
            </GlassButton>
            {(hasPassword || hasSession || loggedIn) && (
              <GlassButton disabled={busy} onClick={onClear} style={{ fontSize: 12 }}>清除登录</GlassButton>
            )}
          </div>
          <div style={{ fontSize: 11, color: "var(--text-disabled)" }}>值提交后立即清空。</div>
        </div>
      )}
    </div>
  )
}
