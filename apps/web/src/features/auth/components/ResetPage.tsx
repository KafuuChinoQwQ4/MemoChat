/** Reset password form */
import { useState } from "react"
import { useNavigate } from "react-router-dom"
import { getGateway } from "@/shared/gateway/ClientGateway"
import { createAuthApi } from "@/features/auth/api/authApi"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { Spinner } from "@/shared/ui/primitives/Spinner"

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
    if (!email) { setError("请输入邮箱"); return }
    try {
      const res = await api.getVarifyCode(email)
      if (res.error !== 0) setError("发送验证码失败")
      else setError(null)
    } catch { setError("网络错误") }
  }

  async function handleSubmit(e: React.FormEvent) {
    e.preventDefault()
    setLoading(true); setError(null)
    try {
      const res = await api.resetPassword(email, code, newPassword)
      if (res.error === 0) { setSuccess(true) }
      else setError("重置失败，请检查验证码")
    } catch { setError("网络错误") } finally { setLoading(false) }
  }

  if (success) {
    return (
      <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }}>
        <GlassSurface elevated style={{ padding: 32, borderRadius: "var(--panel-radius)", textAlign: "center" }}>
          <p style={{ fontSize: 16, marginBottom: 16 }}>密码已重置 ✅</p>
          <GlassButton variant="primary" onClick={() => navigate("/login")}>去登录</GlassButton>
        </GlassSurface>
      </div>
    )
  }

  return (
    <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center", background: "var(--surface-bg)" }}>
      <GlassSurface elevated style={{ width: 360, padding: 32, borderRadius: "var(--panel-radius)" }}>
        <h1 style={{ fontSize: 22, fontWeight: 700, marginBottom: 24, textAlign: "center" }}>重置密码</h1>
        <form onSubmit={(e) => { void handleSubmit(e) }} style={{ display: "flex", flexDirection: "column", gap: 14 }}>
          <div style={{ display: "flex", gap: 8 }}>
            <GlassTextField label="邮箱" type="email" value={email} onChange={(e) => setEmail(e.target.value)} required style={{ flex: 1 }} />
            <GlassButton type="button" onClick={() => { void sendCode() }} style={{ marginTop: 20 }}>获取验证码</GlassButton>
          </div>
          <GlassTextField label="验证码" value={code} onChange={(e) => setCode(e.target.value)} required />
          <GlassTextField label="新密码" type="password" value={newPassword} onChange={(e) => setNewPassword(e.target.value)} required />
          {error && <p style={{ color: "var(--color-badge)", fontSize: 13 }}>{error}</p>}
          <GlassButton type="submit" variant="primary" disabled={loading}>
            {loading ? <Spinner size={18} color="#fff" /> : "重置密码"}
          </GlassButton>
        </form>
        <button onClick={() => navigate("/login")} style={{ marginTop: 16, background: "none", border: "none", color: "var(--text-secondary)", cursor: "pointer", display: "block", margin: "16px auto 0", fontSize: 13 }}>
          返回登录
        </button>
      </GlassSurface>
    </div>
  )
}
