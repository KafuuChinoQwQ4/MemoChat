import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** Login form component */
import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { postLoginBootstrap } from "@/app/bootstrap/postLoginBootstrap";
import { useAuthStore } from "@/features/auth/store/authStore";
import { GlassTextField } from "@/shared/ui/glass/GlassTextField";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { Spinner } from "@/shared/ui/primitives/Spinner";
export function LoginPage() {
    const navigate = useNavigate();
    const { loading, error, setLoading, setError, reset } = useAuthStore();
    const [email, setEmail] = useState("");
    const [password, setPassword] = useState("");
    async function handleSubmit(e) {
        e.preventDefault();
        reset();
        setLoading(true);
        try {
            await postLoginBootstrap({ email, password });
            navigate("/app/chat", { replace: true });
        }
        catch (err) {
            setError(err instanceof Error ? err.message : "登录失败，请重试");
        }
        finally {
            setLoading(false);
        }
    }
    return (_jsx("div", { style: {
            height: "100%",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            background: "var(--surface-bg)",
        }, children: _jsxs(GlassSurface, { elevated: true, style: { width: 360, padding: 32, borderRadius: "var(--panel-radius)" }, children: [_jsx("h1", { style: { fontSize: 22, fontWeight: 700, marginBottom: 24, textAlign: "center", color: "var(--text-primary)" }, children: "\u767B\u5F55 MemoChat" }), _jsxs("form", { onSubmit: (e) => { void handleSubmit(e); }, style: { display: "flex", flexDirection: "column", gap: 16 }, children: [_jsx(GlassTextField, { label: "\u90AE\u7BB1", type: "email", value: email, onChange: (e) => setEmail(e.target.value), placeholder: "your@email.com", required: true, autoFocus: true }), _jsx(GlassTextField, { label: "\u5BC6\u7801", type: "password", value: password, onChange: (e) => setPassword(e.target.value), placeholder: "\u8BF7\u8F93\u5165\u5BC6\u7801", required: true }), error && (_jsx("p", { style: { color: "var(--color-badge)", fontSize: 13, margin: 0 }, children: error })), _jsx(GlassButton, { type: "submit", variant: "primary", disabled: loading, style: { marginTop: 8 }, children: loading ? _jsx(Spinner, { size: 18, color: "#fff" }) : "登录" })] }), _jsxs("div", { style: { marginTop: 16, display: "flex", gap: 8, justifyContent: "center", fontSize: 13 }, children: [_jsx("button", { onClick: () => navigate("/register"), style: { background: "none", border: "none", color: "var(--color-brand-green)", cursor: "pointer" }, children: "\u6CE8\u518C\u8D26\u53F7" }), _jsx("span", { style: { color: "var(--text-disabled)" }, children: "\u00B7" }), _jsx("button", { onClick: () => navigate("/reset"), style: { background: "none", border: "none", color: "var(--text-secondary)", cursor: "pointer" }, children: "\u5FD8\u8BB0\u5BC6\u7801" })] })] }) }));
}
