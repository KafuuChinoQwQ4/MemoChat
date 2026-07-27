/**
 * R18AccountOrCookieLoginPanel — 账密登录 + Cookie 登录 双 tab 面板
 *
 * 用于 nhentai 和 hanime1：支持用户名密码直接登录，也支持粘贴 Cookie。
 * 两种方式都可用时，优先推荐账密（服务端代为登录，自动获取 session）。
 */
import { useCallback, useState } from "react"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { R18PasswordField } from "./R18PasswordField"
import {
  R18CookieFieldLoginPanel,
  type CookieFieldDef,
} from "./R18CookieFieldLoginPanel"

// ─── Tab ──────────────────────────────────────────────────────────────────────

function ModeTab({ label, active, onClick }: { label: string; active: boolean; onClick: () => void }) {
  return (
    <button
      type="button"
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

// ─── Types ────────────────────────────────────────────────────────────────────

export interface R18AccountOrCookieLoginPanelProps {
  sourceId: string
  sourceName: string
  cookieFields: CookieFieldDef[]
  cookieHelpText?: string
  /** Shown below the username field */
  loginHint?: string
  loggedIn: boolean
  hasSession?: boolean
  hasPassword?: boolean
  busy: boolean
  draft: { username: string; password: string }
  onDraftChange: (field: "username" | "password", value: string) => void
  onSave: () => void
  onLogin: () => void
  onClear: () => void
  onImportCookieHeader: (sourceId: string, cookieHeader: string) => Promise<void>
}

// ─── Component ────────────────────────────────────────────────────────────────

export function R18AccountOrCookieLoginPanel({
  sourceId,
  sourceName,
  cookieFields,
  cookieHelpText,
  loginHint,
  loggedIn,
  hasSession,
  hasPassword,
  busy,
  draft,
  onDraftChange,
  onSave,
  onLogin,
  onClear,
  onImportCookieHeader,
}: R18AccountOrCookieLoginPanelProps) {
  const [mode, setMode] = useState<"password" | "cookie">("password")
  const [error, setError] = useState("")

  const handleSave = useCallback(async () => {
    setError("")
    try { onSave() } catch (err) {
      setError(err instanceof Error ? err.message : "保存失败")
    }
  }, [onSave])

  return (
    <div style={{ display: "grid", gap: 10 }}>
      {/* Description */}
      <div style={{ fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.55 }}>
        支持两种方式：① 账号密码登录（服务端代为请求） ② 手动粘贴 Cookie
      </div>

      {/* Mode tabs */}
      <div style={{ display: "flex", gap: 6 }}>
        <ModeTab label="账密登录" active={mode === "password"} onClick={() => { setMode("password"); setError("") }} />
        <ModeTab label="Cookie 登录" active={mode === "cookie"}   onClick={() => { setMode("cookie");   setError("") }} />
      </div>

      {/* ── 账密模式 ── */}
      {mode === "password" && (
        <div style={{ display: "grid", gap: 8 }}>
          {loginHint && (
            <div style={{ fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.55 }}>
              {loginHint}
            </div>
          )}
          <GlassTextField
            value={draft.username}
            onChange={(e) => onDraftChange("username", e.target.value)}
            placeholder={`${sourceName} 账号 / 邮箱`}
            autoComplete="username"
          />
          <R18PasswordField
            value={draft.password}
            onChange={(e) => onDraftChange("password", e.target.value)}
            placeholder={hasPassword ? "密码（留空保留已保存密码）" : "密码"}
            autoComplete="current-password"
          />
          {error && (
            <div style={{ fontSize: 12, color: "var(--color-danger, #ef4444)" }}>{error}</div>
          )}
          <div style={{ display: "flex", flexWrap: "wrap", gap: 8 }}>
            <GlassButton
              variant="primary"
              disabled={busy}
              onClick={handleSave}
              style={{ fontSize: 12 }}
            >
              {busy ? "处理中…" : "保存并登录"}
            </GlassButton>
            {(hasPassword || hasSession) && (
              <GlassButton disabled={busy} onClick={onLogin} style={{ fontSize: 12 }}>
                重新登录
              </GlassButton>
            )}
            {(hasPassword || hasSession || loggedIn) && (
              <GlassButton disabled={busy} onClick={onClear} style={{ fontSize: 12 }}>
                清除
              </GlassButton>
            )}
          </div>
          <div style={{ fontSize: 11, color: "var(--text-disabled)", lineHeight: 1.5 }}>
            服务端会向 {sourceName} 发起登录请求并自动获取 Session Cookie。
          </div>
        </div>
      )}

      {/* ── Cookie 模式 ── */}
      {mode === "cookie" && (
        <R18CookieFieldLoginPanel
          sourceId={sourceId}
          sourceName={sourceName}
          fields={cookieFields}
          {...(cookieHelpText !== undefined ? { helpText: cookieHelpText } : {})}
          loggedIn={loggedIn}
          {...(hasSession !== undefined ? { hasSession } : {})}
          busy={busy}
          onImportCookieHeader={onImportCookieHeader}
          onClear={onClear}
        />
      )}
    </div>
  )
}
