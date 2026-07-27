/**
 * R18CookieFieldLoginPanel — generic labeled-field cookie login panel.
 *
 * Each cookie key is shown as a read-only label; the user pastes only the value.
 * Used by nhentai (sessionid / csrftoken) and hanime1.me (token / remember_token).
 *
 * On submit the panel assembles a "key=value; key2=value2" cookie header string
 * and calls onImportCookieHeader.
 */
import { useCallback, useState } from "react"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"

// ─── Types ────────────────────────────────────────────────────────────────────

export interface CookieFieldDef {
  name: string          // cookie name, shown as a non-editable label
  placeholder?: string  // hint text for the value input
  required?: boolean    // validation: must be non-empty to submit
}

export interface R18CookieFieldLoginPanelProps {
  sourceId: string
  sourceName: string
  fields: CookieFieldDef[]
  helpText?: string
  loggedIn: boolean
  hasSession?: boolean
  busy: boolean
  onImportCookieHeader: (sourceId: string, cookieHeader: string) => Promise<void>
  onClear: () => void
}

// ─── Component ────────────────────────────────────────────────────────────────

export function R18CookieFieldLoginPanel({
  sourceId,
  sourceName,
  fields,
  helpText,
  loggedIn,
  hasSession,
  busy,
  onImportCookieHeader,
  onClear,
}: R18CookieFieldLoginPanelProps) {
  const [values, setValues] = useState<Record<string, string>>(() =>
    Object.fromEntries(fields.map((f) => [f.name, ""])),
  )
  const [submitError, setSubmitError] = useState("")
  const [done, setDone] = useState(false)

  const setValue = useCallback((name: string, value: string) => {
    setValues((prev) => ({ ...prev, [name]: value }))
    setSubmitError("")
  }, [])

  const handleSubmit = useCallback(async () => {
    // Validate required fields.
    for (const field of fields) {
      if (field.required !== false && !(values[field.name] ?? "").trim()) {
        setSubmitError(`请填写 ${field.name}`)
        return
      }
    }

    // Assemble cookie header: "name=value; name2=value2"
    const parts: string[] = []
    for (const field of fields) {
      const val = (values[field.name] ?? "").trim()
      if (val) parts.push(`${field.name}=${val}`)
    }
    if (parts.length === 0) {
      setSubmitError("请至少填写一个 Cookie 值")
      return
    }

    setSubmitError("")
    try {
      await onImportCookieHeader(sourceId, parts.join("; "))
      // Clear value inputs after success.
      setValues(Object.fromEntries(fields.map((f) => [f.name, ""])))
      setDone(true)
    } catch (err) {
      setSubmitError(err instanceof Error ? err.message : "导入失败")
    }
  }, [values, fields, sourceId, onImportCookieHeader])

  const handleReset = useCallback(() => {
    setDone(false)
    setSubmitError("")
    setValues(Object.fromEntries(fields.map((f) => [f.name, ""])))
  }, [fields])

  if (done) {
    return (
      <div style={{ display: "grid", gap: 8 }}>
        <div style={{
          padding: "10px 12px", borderRadius: 8, fontSize: 12,
          background: "color-mix(in srgb, var(--color-success, #22c55e) 12%, transparent)",
          border: "1px solid color-mix(in srgb, var(--color-success, #22c55e) 35%, transparent)",
          color: "var(--color-success, #16a34a)",
        }}>
          ✓ Cookie 已成功导入
        </div>
        <div style={{ display: "flex", gap: 8, flexWrap: "wrap" }}>
          <GlassButton disabled={busy} onClick={onClear} style={{ fontSize: 12 }}>清除登录</GlassButton>
          <GlassButton disabled={busy} onClick={handleReset} style={{ fontSize: 12 }}>重新登录</GlassButton>
        </div>
      </div>
    )
  }

  return (
    <div style={{ display: "grid", gap: 10 }}>
      {helpText && (
        <div style={{ fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.6 }}>
          {helpText}
        </div>
      )}

      <div style={{ fontSize: 11, color: "var(--text-secondary)", lineHeight: 1.5 }}>
        在浏览器中登录 <strong>{sourceName}</strong> 后，按 <code>F12</code> → Application → Cookies，复制以下字段的值粘贴到对应输入框中。
      </div>

      {/* Labeled cookie value fields */}
      <div style={{ display: "grid", gap: 8 }}>
        {fields.map((field) => (
          <div key={field.name} style={{ display: "grid", gap: 4 }}>
            <div style={{
              display: "flex",
              alignItems: "center",
              gap: 8,
            }}>
              {/* Cookie name — read-only label */}
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
                userSelect: "all",
              }}>
                {field.name}
              </div>
              <span style={{ color: "var(--text-disabled)", fontSize: 14 }}>=</span>
              {/* Value input */}
              <div style={{ flex: 1 }}>
                <GlassTextField
                  type="password"
                  value={values[field.name] ?? ""}
                  onChange={(e) => setValue(field.name, e.target.value)}
                  placeholder={field.placeholder ?? "粘贴 value"}
                  autoComplete="off"
                />
              </div>
            </div>
          </div>
        ))}
      </div>

      {submitError && (
        <div style={{ fontSize: 12, color: "var(--color-danger, #ef4444)" }}>{submitError}</div>
      )}

      <div style={{ display: "flex", gap: 8, flexWrap: "wrap" }}>
        <GlassButton
          variant="primary"
          disabled={busy}
          onClick={handleSubmit}
          style={{ fontSize: 12 }}
        >
          {busy ? "导入中…" : "导入 Cookie"}
        </GlassButton>
        {(hasSession || loggedIn) && (
          <GlassButton disabled={busy} onClick={onClear} style={{ fontSize: 12 }}>
            清除登录
          </GlassButton>
        )}
      </div>

      <div style={{ fontSize: 11, color: "var(--text-disabled)", lineHeight: 1.5 }}>
        值提交后立即清空，不保留在页面中。
      </div>
    </div>
  )
}

// ─── Pre-configured field sets for each source ───────────────────────────────

export const NHENTAI_COOKIE_FIELDS: CookieFieldDef[] = [
  { name: "refresh_token", placeholder: "粘贴 refresh_token 的值", required: true },
]

export const HANIME1_COOKIE_FIELDS: CookieFieldDef[] = [
  { name: "remember_token", placeholder: "记住登录的 token", required: true },
]

export const HANIMEONE_COOKIE_FIELDS: CookieFieldDef[] = [
  { name: "remember_token", placeholder: "与 hanime1.me 共享同一 remember_token", required: true },
]
