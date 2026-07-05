import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** Register form component */
import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { createAuthApi } from "@/features/auth/api/authApi";
import { GlassTextField } from "@/shared/ui/glass/GlassTextField";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { Spinner } from "@/shared/ui/primitives/Spinner";
export function RegisterPage() {
    const navigate = useNavigate();
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState(null);
    const [codeSent, setCodeSent] = useState(false);
    const [email, setEmail] = useState("");
    const [code, setCode] = useState("");
    const [name, setName] = useState("");
    const [password, setPassword] = useState("");
    const api = createAuthApi(getGateway().http);
    async function sendCode() {
        if (!email) {
            setError("请输入邮箱");
            return;
        }
        try {
            const res = await api.getVarifyCode(email);
            if (res.error === 0) {
                setCodeSent(true);
                setError(null);
            }
            else
                setError("发送验证码失败");
        }
        catch {
            setError("网络错误");
        }
    }
    async function handleSubmit(e) {
        e.preventDefault();
        setLoading(true);
        setError(null);
        try {
            const res = await api.register({ email, password, code, name });
            if (res.error === 0)
                navigate("/login", { replace: true });
            else
                setError("注册失败，请检查验证码");
        }
        catch {
            setError("网络错误");
        }
        finally {
            setLoading(false);
        }
    }
    return (_jsx("div", { style: { height: "100%", display: "flex", alignItems: "center", justifyContent: "center", background: "var(--surface-bg)" }, children: _jsxs(GlassSurface, { elevated: true, style: { width: 360, padding: 32, borderRadius: "var(--panel-radius)" }, children: [_jsx("h1", { style: { fontSize: 22, fontWeight: 700, marginBottom: 24, textAlign: "center" }, children: "\u6CE8\u518C\u8D26\u53F7" }), _jsxs("form", { onSubmit: (e) => { void handleSubmit(e); }, style: { display: "flex", flexDirection: "column", gap: 14 }, children: [_jsx(GlassTextField, { label: "\u7528\u6237\u540D", value: name, onChange: (e) => setName(e.target.value), placeholder: "\u6635\u79F0", required: true }), _jsxs("div", { style: { display: "flex", gap: 8 }, children: [_jsx(GlassTextField, { label: "\u90AE\u7BB1", type: "email", value: email, onChange: (e) => setEmail(e.target.value), placeholder: "your@email.com", required: true, style: { flex: 1 } }), _jsx(GlassButton, { type: "button", onClick: () => { void sendCode(); }, style: { marginTop: 20, whiteSpace: "nowrap" }, children: codeSent ? "重发" : "获取验证码" })] }), _jsx(GlassTextField, { label: "\u9A8C\u8BC1\u7801", value: code, onChange: (e) => setCode(e.target.value), placeholder: "6\u4F4D\u9A8C\u8BC1\u7801", required: true }), _jsx(GlassTextField, { label: "\u5BC6\u7801", type: "password", value: password, onChange: (e) => setPassword(e.target.value), placeholder: "\u81F3\u5C118\u4F4D", required: true }), error && _jsx("p", { style: { color: "var(--color-badge)", fontSize: 13, margin: 0 }, children: error }), _jsx(GlassButton, { type: "submit", variant: "primary", disabled: loading, style: { marginTop: 4 }, children: loading ? _jsx(Spinner, { size: 18, color: "#fff" }) : "注册" })] }), _jsx("button", { onClick: () => navigate("/login"), style: { marginTop: 16, background: "none", border: "none", color: "var(--color-brand-green)", cursor: "pointer", display: "block", margin: "16px auto 0", fontSize: 13 }, children: "\u5DF2\u6709\u8D26\u53F7\uFF1F\u767B\u5F55" })] }) }));
}
