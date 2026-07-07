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
function BrandMark() {
    return (_jsxs("svg", { width: "48", height: "48", viewBox: "0 0 48 48", fill: "none", "aria-hidden": true, style: { display: "block" }, children: [_jsx("rect", { width: "48", height: "48", rx: "14", fill: "var(--color-brand-green)" }), _jsx("path", { d: "M11 15.5C11 13.567 12.567 12 14.5 12H33.5C35.433 12 37 13.567 37 15.5V28.5C37 30.433 35.433 32 33.5 32H26L19 37V32H14.5C12.567 32 11 30.433 11 28.5V15.5Z", fill: "white", fillOpacity: "0.95" }), _jsx("circle", { cx: "18.5", cy: "22", r: "2", fill: "var(--color-brand-green)" }), _jsx("circle", { cx: "24", cy: "22", r: "2", fill: "var(--color-brand-green)" }), _jsx("circle", { cx: "29.5", cy: "22", r: "2", fill: "var(--color-brand-green)" })] }));
}
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
    return (_jsx("div", { style: {
            height: "100%",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
        }, children: _jsxs(GlassSurface, { elevated: true, className: "enter-fade-up", style: { width: 380, padding: "36px 32px 28px", borderRadius: "var(--panel-radius)" }, children: [_jsxs("div", { style: {
                        display: "flex",
                        flexDirection: "column",
                        alignItems: "center",
                        gap: 10,
                        marginBottom: 28,
                    }, children: [_jsx(BrandMark, {}), _jsxs("div", { style: { textAlign: "center" }, children: [_jsx("h1", { style: { fontSize: 20, fontWeight: 700, color: "var(--text-primary)", margin: 0 }, children: "\u521B\u5EFA\u8D26\u53F7" }), _jsx("p", { style: { fontSize: 13, color: "var(--text-secondary)", margin: "4px 0 0" }, children: "\u52A0\u5165 MemoChat\uFF0C\u5F00\u59CB\u804A\u5929" })] })] }), _jsxs("form", { onSubmit: (e) => { void handleSubmit(e); }, style: { display: "flex", flexDirection: "column", gap: 14 }, children: [_jsx(GlassTextField, { label: "\u7528\u6237\u540D", value: name, onChange: (e) => setName(e.target.value), placeholder: "\u4F60\u7684\u6635\u79F0", required: true, autoFocus: true }), _jsxs("div", { style: { display: "flex", gap: 8, alignItems: "flex-end" }, children: [_jsx(GlassTextField, { label: "\u90AE\u7BB1", type: "email", value: email, onChange: (e) => setEmail(e.target.value), placeholder: "your@email.com", required: true, style: { flex: 1 } }), _jsx(GlassButton, { type: "button", onClick: () => { void sendCode(); }, style: { flexShrink: 0, whiteSpace: "nowrap", marginBottom: 1 }, children: codeSent ? "重发" : "获取验证码" })] }), _jsx(GlassTextField, { label: "\u9A8C\u8BC1\u7801", value: code, onChange: (e) => setCode(e.target.value), placeholder: "6 \u4F4D\u9A8C\u8BC1\u7801", required: true }), _jsx(GlassTextField, { label: "\u5BC6\u7801", type: "password", value: password, onChange: (e) => setPassword(e.target.value), placeholder: "\u81F3\u5C11 8 \u4F4D", required: true }), error && (_jsx("p", { style: {
                                color: "var(--color-badge)",
                                fontSize: 13,
                                margin: 0,
                                padding: "8px 10px",
                                borderRadius: 8,
                                background: "rgba(232,65,65,0.07)",
                                border: "1px solid rgba(232,65,65,0.16)",
                            }, children: error })), _jsx(GlassButton, { type: "submit", variant: "primary", disabled: loading, style: { marginTop: 4 }, children: loading ? _jsx(Spinner, { size: 18, color: "#fff" }) : "注册" })] }), _jsxs("div", { style: {
                        marginTop: 20,
                        paddingTop: 16,
                        borderTop: "1px solid var(--divider)",
                        textAlign: "center",
                        fontSize: 13,
                        color: "var(--text-secondary)",
                    }, children: ["\u5DF2\u6709\u8D26\u53F7\uFF1F", " ", _jsx("button", { onClick: () => navigate("/login"), style: {
                                background: "none",
                                border: "none",
                                color: "var(--color-brand-green)",
                                cursor: "pointer",
                                fontWeight: 500,
                                padding: 0,
                            }, children: "\u7ACB\u5373\u767B\u5F55" })] })] }) }));
}
