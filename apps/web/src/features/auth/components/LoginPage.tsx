/** Login form component */
import { useState } from "react"
import { useNavigate } from "react-router-dom"
import { postLoginBootstrap } from "@/app/bootstrap/postLoginBootstrap"
import { useAuthStore } from "@/features/auth/store/authStore"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { Spinner } from "@/shared/ui/primitives/Spinner"

export function LoginPage() {
  const navigate = useNavigate()
  const { loading, error, setLoading, setError, reset } = useAuthStore()
  const [email, setEmail] = useState("")
  const [password, setPassword] = useState("")

  async function handleSubmit(e: React.FormEvent) {
    e.preventDefault()
    reset()
    setLoading(true)
    try {
      await postLoginBootstrap({ email, password })
      navigate("/app/chat", { replace: true })
    } catch (err) {
      setError(err instanceof Error ? err.message : "登录失败，请重试")
    } finally {
      setLoading(false)
    }
  }

  return (
    <div style={{
      height: "100%",
      display: "flex",
      alignItems: "center",
      justifyContent: "center",
      background: "var(--surface-bg)",
    }}>
      <GlassSurface elevated style={{ width: 360, padding: 32, borderRadius: "var(--panel-radius)" }}>
        <h1 style={{ fontSize: 22, fontWeight: 700, marginBottom: 24, textAlign: "center", color: "var(--text-primary)" }}>
          登录 MemoChat
        </h1>

        <form onSubmit={(e) => { void handleSubmit(e) }} style={{ display: "flex", flexDirection: "column", gap: 16 }}>
          <GlassTextField
            label="邮箱"
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            placeholder="your@email.com"
            required
            autoFocus
          />
          <GlassTextField
            label="密码"
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            placeholder="请输入密码"
            required
          />

          {error && (
            <p style={{ color: "var(--color-badge)", fontSize: 13, margin: 0 }}>{error}</p>
          )}

          <GlassButton type="submit" variant="primary" disabled={loading} style={{ marginTop: 8 }}>
            {loading ? <Spinner size={18} color="#fff" /> : "登录"}
          </GlassButton>
        </form>

        <div style={{ marginTop: 16, display: "flex", gap: 8, justifyContent: "center", fontSize: 13 }}>
          <button onClick={() => navigate("/register")} style={{ background: "none", border: "none", color: "var(--color-brand-green)", cursor: "pointer" }}>
            注册账号
          </button>
          <span style={{ color: "var(--text-disabled)" }}>·</span>
          <button onClick={() => navigate("/reset")} style={{ background: "none", border: "none", color: "var(--text-secondary)", cursor: "pointer" }}>
            忘记密码
          </button>
        </div>
      </GlassSurface>
    </div>
  )
}
