/** Register form component */
import { useState } from "react"
import { useNavigate } from "react-router-dom"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { createAuthApi } from "@/features/auth/api/authApi"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { Spinner } from "@/shared/ui/primitives/Spinner"

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
    <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center", background: "var(--surface-bg)" }}>
      <GlassSurface elevated style={{ width: 360, padding: 32, borderRadius: "var(--panel-radius)" }}>
        <h1 style={{ fontSize: 22, fontWeight: 700, marginBottom: 24, textAlign: "center" }}>注册账号</h1>
        <form onSubmit={(e) => { void handleSubmit(e) }} style={{ display: "flex", flexDirection: "column", gap: 14 }}>
          <GlassTextField label="用户名" value={name} onChange={(e) => setName(e.target.value)} placeholder="昵称" required />
          <div style={{ display: "flex", gap: 8 }}>
            <GlassTextField label="邮箱" type="email" value={email} onChange={(e) => setEmail(e.target.value)} placeholder="your@email.com" required style={{ flex: 1 }} />
            <GlassButton type="button" onClick={() => { void sendCode() }} style={{ marginTop: 20, whiteSpace: "nowrap" }}>
              {codeSent ? "重发" : "获取验证码"}
            </GlassButton>
          </div>
          <GlassTextField label="验证码" value={code} onChange={(e) => setCode(e.target.value)} placeholder="6位验证码" required />
          <GlassTextField label="密码" type="password" value={password} onChange={(e) => setPassword(e.target.value)} placeholder="至少8位" required />
          {error && <p style={{ color: "var(--color-badge)", fontSize: 13, margin: 0 }}>{error}</p>}
          <GlassButton type="submit" variant="primary" disabled={loading} style={{ marginTop: 4 }}>
            {loading ? <Spinner size={18} color="#fff" /> : "注册"}
          </GlassButton>
        </form>
        <button onClick={() => navigate("/login")} style={{ marginTop: 16, background: "none", border: "none", color: "var(--color-brand-green)", cursor: "pointer", display: "block", margin: "16px auto 0", fontSize: 13 }}>
          已有账号？登录
        </button>
      </GlassSurface>
    </div>
  )
}
