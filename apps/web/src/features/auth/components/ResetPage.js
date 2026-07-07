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
function BrandMark() {
    return (_jsxs("svg", { width: "48", height: "48", viewBox: "0 0 48 48", fill: "none", "aria-hidden": true, style: { display: "block" }, children: [_jsx("rect", { width: "48", height: "48", rx: "14", fill: "var(--color-brand-green)" }), _jsx("path", { d: "M11 15.5C11 13.567 12.567 12 14.5 12H33.5C35.433 12 37 13.567 37 15.5V28.5C37 30.433 35.433 32 33.5 32H26L19 37V32H14.5C12.567 32 11 30.433 11 28.5V15.5Z", fill: "white", fillOpacity: "0.95" }), _jsx("circle", { cx: "18.5", cy: "22", r: "2", fill: "var(--color-brand-green)" }), _jsx("circle", { cx: "24", cy: "22", r: "2", fill: "var(--color-brand-green)" }), _jsx("circle", { cx: "29.5", cy: "22", r: "2", fill: "var(--color-brand-green)" })] }));
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
    const cardStyle = {
        width: 360,
        padding: "36px 32px 28px",
        borderRadius: "var(--panel-radius)",
    };
    if (success) {
        return (_jsx("div", { style: { height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }, children: _jsx(GlassSurface, { elevated: true, className: "enter-fade-up", style: cardStyle, children: _jsxs("div", { style: {
                        display: "flex",
                        flexDirection: "column",
                        alignItems: "center",
                        gap: 14,
                        textAlign: "center",
                    }, children: [_jsx("div", { style: {
                                width: 56,
                                height: 56,
                                borderRadius: "50%",
                                background: "rgba(7,193,96,0.12)",
                                border: "1px solid rgba(7,193,96,0.30)",
                                display: "grid",
                                placeItems: "center",
                            }, children: _jsxs("svg", { width: "28", height: "28", viewBox: "0 0 24 24", fill: "none", stroke: "var(--color-brand-green)", strokeWidth: "2.2", strokeLinecap: "round", strokeLinejoin: "round", "aria-hidden": true, children: [_jsx("circle", { cx: "12", cy: "12", r: "8" }), _jsx("path", { d: "m8.6 12 2.2 2.2 4.8-5" })] }) }), _jsxs("div", { children: [_jsx("h2", { style: { fontSize: 18, fontWeight: 700, color: "var(--text-primary)", margin: 0 }, children: "\u5BC6\u7801\u5DF2\u91CD\u7F6E" }), _jsx("p", { style: { fontSize: 13, color: "var(--text-secondary)", margin: "6px 0 0" }, children: "\u8BF7\u4F7F\u7528\u65B0\u5BC6\u7801\u767B\u5F55" })] }), _jsx(GlassButton, { variant: "primary", style: { width: "100%", marginTop: 4 }, onClick: () => { void navigate("/login"); }, children: "\u53BB\u767B\u5F55" })] }) }) }));
    }
    return (_jsx("div", { style: {
            height: "100%",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
        }, children: _jsxs(GlassSurface, { elevated: true, className: "enter-fade-up", style: cardStyle, children: [_jsxs("div", { style: {
                        display: "flex",
                        flexDirection: "column",
                        alignItems: "center",
                        gap: 10,
                        marginBottom: 28,
                    }, children: [_jsx(BrandMark, {}), _jsxs("div", { style: { textAlign: "center" }, children: [_jsx("h1", { style: { fontSize: 20, fontWeight: 700, color: "var(--text-primary)", margin: 0 }, children: "\u91CD\u7F6E\u5BC6\u7801" }), _jsx("p", { style: { fontSize: 13, color: "var(--text-secondary)", margin: "4px 0 0" }, children: "\u901A\u8FC7\u90AE\u7BB1\u9A8C\u8BC1\u91CD\u7F6E\u4F60\u7684\u5BC6\u7801" })] })] }), _jsxs("form", { onSubmit: (e) => { void handleSubmit(e); }, style: { display: "flex", flexDirection: "column", gap: 14 }, children: [_jsxs("div", { style: { display: "flex", gap: 8, alignItems: "flex-end" }, children: [_jsx(GlassTextField, { label: "\u90AE\u7BB1", type: "email", value: email, onChange: (e) => setEmail(e.target.value), required: true, style: { flex: 1 } }), _jsx(GlassButton, { type: "button", onClick: () => { void sendCode(); }, style: { flexShrink: 0, whiteSpace: "nowrap", marginBottom: 1 }, children: "\u83B7\u53D6\u9A8C\u8BC1\u7801" })] }), _jsx(GlassTextField, { label: "\u9A8C\u8BC1\u7801", value: code, onChange: (e) => setCode(e.target.value), required: true }), _jsx(GlassTextField, { label: "\u65B0\u5BC6\u7801", type: "password", value: newPassword, onChange: (e) => setNewPassword(e.target.value), placeholder: "\u81F3\u5C11 8 \u4F4D", required: true }), error && (_jsx("p", { style: {
                                color: "var(--color-badge)",
                                fontSize: 13,
                                margin: 0,
                                padding: "8px 10px",
                                borderRadius: 8,
                                background: "rgba(232,65,65,0.07)",
                                border: "1px solid rgba(232,65,65,0.16)",
                            }, children: error })), _jsx(GlassButton, { type: "submit", variant: "primary", disabled: loading, style: { marginTop: 4 }, children: loading ? _jsx(Spinner, { size: 18, color: "#fff" }) : "重置密码" })] }), _jsx("div", { style: {
                        marginTop: 20,
                        paddingTop: 16,
                        borderTop: "1px solid var(--divider)",
                        textAlign: "center",
                        fontSize: 13,
                    }, children: _jsx("button", { onClick: () => { void navigate("/login"); }, style: {
                            background: "none",
                            border: "none",
                            color: "var(--text-secondary)",
                            cursor: "pointer",
                            padding: 0,
                        }, children: "\u2190 \u8FD4\u56DE\u767B\u5F55" }) })] }) }));
}
