/** Register form component */
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

export function RegisterPage() {
  const navigate = useNavigate()
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [codeSent, setCodeSent] = useState(false)
  const [email, setEmail] = useState("")
  const [code, setCode] = useState("")
  const [name, setName] = useState("")
  const [password, setPassword] = useState("")

  const api = createAuthApi(getGateway().http)

  async function sendCode() {
    if (!email) { setError("请输入邮箱"); return }
    try {
      const res = await api.getVarifyCode(email)
      if (res.error === 0) { setCodeSent(true); setError(null) }
      else setError("发送验证码失败")
    } catch { setError("网络错误") }
  }

  async function handleSubmit(e: React.FormEvent) {
    e.preventDefault()
    setLoading(true); setError(null)
    try {
      const res = await api.register({ email, password, code, name })
      if (res.error === 0) navigate("/login", { replace: true })
      else setError("注册失败，请检查验证码")
    } catch { setError("网络错误") } finally { setLoading(false) }
  }

  return (
    <div style={{
      height: "100%",
      display: "flex",
      alignItems: "center",
      justifyContent: "center",
    }}>
      <GlassSurface
        elevated
        className="enter-fade-up"
        style={{ width: 380, padding: "36px 32px 28px", borderRadius: "var(--panel-radius)" }}
      >
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
              创建账号
            </h1>
            <p style={{ fontSize: 13, color: "var(--text-secondary)", margin: "4px 0 0" }}>
              加入 MemoChat，开始聊天
            </p>
          </div>
        </div>

        {/* Form */}
        <form onSubmit={(e) => { void handleSubmit(e) }} style={{ display: "flex", flexDirection: "column", gap: 14 }}>
          <GlassTextField
            label="用户名"
            value={name}
            onChange={(e) => setName(e.target.value)}
            placeholder="你的昵称"
            required
            autoFocus
          />

          {/* Email + send code in a row */}
          <div style={{ display: "flex", gap: 8, alignItems: "flex-end" }}>
            <GlassTextField
              label="邮箱"
              type="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="your@email.com"
              required
              style={{ flex: 1 }}
            />
            <GlassButton
              type="button"
              onClick={() => { void sendCode() }}
              style={{ flexShrink: 0, whiteSpace: "nowrap", marginBottom: 1 }}
            >
              {codeSent ? "重发" : "获取验证码"}
            </GlassButton>
          </div>

          <GlassTextField
            label="验证码"
            value={code}
            onChange={(e) => setCode(e.target.value)}
            placeholder="6 位验证码"
            required
          />
          <GlassTextField
            label="密码"
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
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
            {loading ? <Spinner size={18} color="#fff" /> : "注册"}
          </GlassButton>
        </form>

        {/* Login link */}
        <div style={{
          marginTop: 20,
          paddingTop: 16,
          borderTop: "1px solid var(--divider)",
          textAlign: "center",
          fontSize: 13,
          color: "var(--text-secondary)",
        }}>
          已有账号？{" "}
          <button
            onClick={() => navigate("/login")}
            style={{
              background: "none",
              border: "none",
              color: "var(--color-brand-green)",
              cursor: "pointer",
              fontWeight: 500,
              padding: 0,
            }}
          >
            立即登录
          </button>
        </div>
      </GlassSurface>
    </div>
  )
}
