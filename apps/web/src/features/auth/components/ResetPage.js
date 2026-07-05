import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** Reset password form */
import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { createAuthApi } from "@/features/auth/api/authApi";
import { GlassTextField } from "@/shared/ui/glass/GlassTextField";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { Spinner } from "@/shared/ui/primitives/Spinner";
function SuccessMark() {
    return (_jsx("span", { style: {
            display: "inline-grid",
            placeItems: "center",
            width: 20,
            height: 20,
            color: "var(--color-brand-green)",
        }, children: _jsxs("svg", { width: "18", height: "18", viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: "2", strokeLinecap: "round", strokeLinejoin: "round", "aria-hidden": true, children: [_jsx("circle", { cx: "12", cy: "12", r: "8" }), _jsx("path", { d: "m8.6 12 2.2 2.2 4.8-5" })] }) }));
}
export function ResetPage() {
    const navigate = useNavigate();
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState(null);
    const [success, setSuccess] = useState(false);
    const [email, setEmail] = useState("");
    const [code, setCode] = useState("");
    const [newPassword, setNewPassword] = useState("");
    const api = createAuthApi(getGateway().http);
    async function sendCode() {
        if (!email) {
            setError("请输入邮箱");
            return;
        }
        try {
            const res = await api.getVarifyCode(email);
            if (res.error !== 0)
                setError("发送验证码失败");
            else
                setError(null);
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
            const res = await api.resetPassword(email, code, newPassword);
            if (res.error === 0) {
                setSuccess(true);
            }
            else
                setError("重置失败，请检查验证码");
        }
        catch {
            setError("网络错误");
        }
        finally {
            setLoading(false);
        }
    }
    if (success) {
        return (_jsx("div", { style: { height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }, children: _jsxs(GlassSurface, { elevated: true, style: { padding: 32, borderRadius: "var(--panel-radius)", textAlign: "center" }, children: [_jsxs("p", { style: {
                            fontSize: 16,
                            marginBottom: 16,
                            display: "inline-flex",
                            alignItems: "center",
                            gap: 6,
                        }, children: ["\u5BC6\u7801\u5DF2\u91CD\u7F6E", _jsx(SuccessMark, {})] }), _jsx(GlassButton, { variant: "primary", onClick: () => {
                            void navigate("/login");
                        }, children: "\u53BB\u767B\u5F55" })] }) }));
    }
    return (_jsx("div", { style: {
            height: "100%",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            background: "var(--surface-bg)",
        }, children: _jsxs(GlassSurface, { elevated: true, style: { width: 360, padding: 32, borderRadius: "var(--panel-radius)" }, children: [_jsx("h1", { style: { fontSize: 22, fontWeight: 700, marginBottom: 24, textAlign: "center" }, children: "\u91CD\u7F6E\u5BC6\u7801" }), _jsxs("form", { onSubmit: (e) => {
                        void handleSubmit(e);
                    }, style: { display: "flex", flexDirection: "column", gap: 14 }, children: [_jsxs("div", { style: { display: "flex", gap: 8 }, children: [_jsx(GlassTextField, { label: "\u90AE\u7BB1", type: "email", value: email, onChange: (e) => setEmail(e.target.value), required: true, style: { flex: 1 } }), _jsx(GlassButton, { type: "button", onClick: () => {
                                        void sendCode();
                                    }, style: { marginTop: 20 }, children: "\u83B7\u53D6\u9A8C\u8BC1\u7801" })] }), _jsx(GlassTextField, { label: "\u9A8C\u8BC1\u7801", value: code, onChange: (e) => setCode(e.target.value), required: true }), _jsx(GlassTextField, { label: "\u65B0\u5BC6\u7801", type: "password", value: newPassword, onChange: (e) => setNewPassword(e.target.value), required: true }), error && _jsx("p", { style: { color: "var(--color-badge)", fontSize: 13 }, children: error }), _jsx(GlassButton, { type: "submit", variant: "primary", disabled: loading, children: loading ? _jsx(Spinner, { size: 18, color: "#fff" }) : "重置密码" })] }), _jsx("button", { onClick: () => {
                        void navigate("/login");
                    }, style: {
                        marginTop: 16,
                        background: "none",
                        border: "none",
                        color: "var(--text-secondary)",
                        cursor: "pointer",
                        display: "block",
                        margin: "16px auto 0",
                        fontSize: 13,
                    }, children: "\u8FD4\u56DE\u767B\u5F55" })] }) }));
}
