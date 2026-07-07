/** Reset password form */
import { useState } from "react"
import { useNavigate } from "react-router-dom"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { createAuthApi } from "@/features/auth/api/authApi"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { Spinner } from "@/shared/ui/primitives/Spinner"

function BrandMark() {
  return (
    <svg
      width="48"
      height="48"
      viewBox="0 0 48 48"
      fill="none"
      aria-hidden
      style={{ display: "block" }}
    >
      <rect width="48" height="48" rx="14" fill="var(--color-brand-green)" />
      <path
        d="M11 15.5C11 13.567 12.567 12 14.5 12H33.5C35.433 12 37 13.567 37 15.5V28.5C37 30.433 35.433 32 33.5 32H26L19 37V32H14.5C12.567 32 11 30.433 11 28.5V15.5Z"
        fill="white"
        fillOpacity="0.95"
      />
      <circle cx="18.5" cy="22" r="2" fill="var(--color-brand-green)" />
      <circle cx="24" cy="22" r="2" fill="var(--color-brand-green)" />
      <circle cx="29.5" cy="22" r="2" fill="var(--color-brand-green)" />
    </svg>
  )
}

export function ResetPage() {
  const navigate = useNavigate()
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [success, setSuccess] = useState(false)
  const [email, setEmail] = useState("")
  const [code, setCode] = useState("")
  const [newPassword, setNewPassword] = useState("")

  const api = createAuthApi(getGateway().http)

  async function sendCode() {
    if (!email) {
      setError("请输入邮箱")
      return
    }
    try {
      const res = await api.getVarifyCode(email)
      if (res.error !== 0) setError("发送验证码失败")
      else setError(null)
    } catch {
      setError("网络错误")
    }
  }

  async function handleSubmit(e: React.FormEvent) {
    e.preventDefault()
    setLoading(true)
    setError(null)
    try {
      const res = await api.resetPassword(email, code, newPassword)
      if (res.error === 0) {
        setSuccess(true)
      } else setError("重置失败，请检查验证码")
    } catch {
      setError("网络错误")
    } finally {
      setLoading(false)
    }
  }

  const cardStyle: React.CSSProperties = {
    width: 360,
    padding: "36px 32px 28px",
    borderRadius: "var(--panel-radius)",
  }

  if (success) {
    return (
      <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }}>
        <GlassSurface elevated className="enter-fade-up" style={cardStyle}>
          <div style={{
            display: "flex",
            flexDirection: "column",
            alignItems: "center",
            gap: 14,
            textAlign: "center",
          }}>
            {/* Success icon */}
            <div style={{
              width: 56,
              height: 56,
              borderRadius: "50%",
              background: "rgba(7,193,96,0.12)",
              border: "1px solid rgba(7,193,96,0.30)",
              display: "grid",
              placeItems: "center",
            }}>
              <svg width="28" height="28" viewBox="0 0 24 24" fill="none"
                stroke="var(--color-brand-green)" strokeWidth="2.2"
                strokeLinecap="round" strokeLinejoin="round" aria-hidden>
                <circle cx="12" cy="12" r="8" />
                <path d="m8.6 12 2.2 2.2 4.8-5" />
              </svg>
            </div>
            <div>
              <h2 style={{ fontSize: 18, fontWeight: 700, color: "var(--text-primary)", margin: 0 }}>
                密码已重置
              </h2>
              <p style={{ fontSize: 13, color: "var(--text-secondary)", margin: "6px 0 0" }}>
                请使用新密码登录
              </p>
            </div>
            <GlassButton variant="primary" style={{ width: "100%", marginTop: 4 }}
              onClick={() => { void navigate("/login") }}>
              去登录
            </GlassButton>
          </div>
        </GlassSurface>
      </div>
    )
  }

  return (
    <div style={{
      height: "100%",
      display: "flex",
      alignItems: "center",
      justifyContent: "center",
    }}>
      <GlassSurface elevated className="enter-fade-up" style={cardStyle}>
        {/* Brand header */}
        <div style={{
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          gap: 10,
          marginBottom: 28,
        }}>
          <BrandMark />
          <div style={{ textAlign: "center" }}>
            <h1 style={{ fontSize: 20, fontWeight: 700, color: "var(--text-primary)", margin: 0 }}>
              重置密码
            </h1>
            <p style={{ fontSize: 13, color: "var(--text-secondary)", margin: "4px 0 0" }}>
              通过邮箱验证重置你的密码
            </p>
          </div>
        </div>

        <form
          onSubmit={(e) => { void handleSubmit(e) }}
          style={{ display: "flex", flexDirection: "column", gap: 14 }}
        >
          {/* Email + send code */}
          <div style={{ display: "flex", gap: 8, alignItems: "flex-end" }}>
            <GlassTextField
              label="邮箱"
              type="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              required
              style={{ flex: 1 }}
            />
            <GlassButton
              type="button"
              onClick={() => { void sendCode() }}
              style={{ flexShrink: 0, whiteSpace: "nowrap", marginBottom: 1 }}
            >
              获取验证码
            </GlassButton>
          </div>

          <GlassTextField
            label="验证码"
            value={code}
            onChange={(e) => setCode(e.target.value)}
            required
          />
          <GlassTextField
            label="新密码"
            type="password"
            value={newPassword}
            onChange={(e) => setNewPassword(e.target.value)}
            placeholder="至少 8 位"
            required
          />

          {error && (
            <p style={{
              color: "var(--color-badge)",
              fontSize: 13,
              margin: 0,
              padding: "8px 10px",
              borderRadius: 8,
              background: "rgba(232,65,65,0.07)",
              border: "1px solid rgba(232,65,65,0.16)",
            }}>
              {error}
            </p>
          )}

          <GlassButton type="submit" variant="primary" disabled={loading} style={{ marginTop: 4 }}>
            {loading ? <Spinner size={18} color="#fff" /> : "重置密码"}
          </GlassButton>
        </form>

        {/* Back to login */}
        <div style={{
          marginTop: 20,
          paddingTop: 16,
          borderTop: "1px solid var(--divider)",
          textAlign: "center",
          fontSize: 13,
        }}>
          <button
            onClick={() => { void navigate("/login") }}
            style={{
              background: "none",
              border: "none",
              color: "var(--text-secondary)",
              cursor: "pointer",
              padding: 0,
            }}
          >
            ← 返回登录
          </button>
        </div>
      </GlassSurface>
    </div>
  )
}
