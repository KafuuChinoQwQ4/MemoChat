/** Login form component */
import { useState } from "react"
import { useNavigate } from "react-router-dom"
import { postLoginBootstrap } from "@/app/bootstrap/postLoginBootstrap"
import { useAuthStore } from "@/features/auth/store/authStore"
import { GlassTextField } from "@/shared/ui/glass/GlassTextField"
import { GlassButton } from "@/shared/ui/glass/GlassButton"
import { GlassSurface } from "@/shared/ui/glass/GlassSurface"
import { BrandMark } from "@/shared/ui/primitives/BrandMark"
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
    }}>
      <GlassSurface
        elevated
        className="enter-fade-up"
        style={{ width: 360, padding: "36px 32px 28px", borderRadius: "var(--panel-radius)" }}
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
              欢迎回来
            </h1>
            <p style={{ fontSize: 13, color: "var(--text-secondary)", margin: "4px 0 0" }}>
              登录你的 MemoChat 账号
            </p>
          </div>
        </div>

        {/* Form */}
        <form onSubmit={(e) => { void handleSubmit(e) }} style={{ display: "flex", flexDirection: "column", gap: 14 }}>
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

          {/* Forgot password link — aligned right */}
          <div style={{ textAlign: "right", marginTop: -6 }}>
            <button
              type="button"
              onClick={() => navigate("/reset")}
              style={{
                background: "none",
                border: "none",
                color: "var(--text-secondary)",
                cursor: "pointer",
                fontSize: 12,
                padding: 0,
              }}
            >
              忘记密码？
            </button>
          </div>

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
            {loading ? <Spinner size={18} color="#fff" /> : "登录"}
          </GlassButton>
        </form>

        {/* Register link */}
        <div style={{
          marginTop: 20,
          paddingTop: 16,
          borderTop: "1px solid var(--divider)",
          textAlign: "center",
          fontSize: 13,
          color: "var(--text-secondary)",
        }}>
          还没有账号？{" "}
          <button
            onClick={() => navigate("/register")}
            style={{
              background: "none",
              border: "none",
              color: "var(--color-brand-green)",
              cursor: "pointer",
              fontWeight: 500,
              padding: 0,
            }}
          >
            立即注册
          </button>
        </div>
      </GlassSurface>
    </div>
  )
}
