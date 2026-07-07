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
/** MemoChat bubble-chat logo mark */
function BrandMark() {
    return (_jsxs("svg", { width: "48", height: "48", viewBox: "0 0 48 48", fill: "none", "aria-hidden": true, style: { display: "block" }, children: [_jsx("rect", { width: "48", height: "48", rx: "14", fill: "var(--color-brand-green)" }), _jsx("path", { d: "M11 15.5C11 13.567 12.567 12 14.5 12H33.5C35.433 12 37 13.567 37 15.5V28.5C37 30.433 35.433 32 33.5 32H26L19 37V32H14.5C12.567 32 11 30.433 11 28.5V15.5Z", fill: "white", fillOpacity: "0.95" }), _jsx("circle", { cx: "18.5", cy: "22", r: "2", fill: "var(--color-brand-green)" }), _jsx("circle", { cx: "24", cy: "22", r: "2", fill: "var(--color-brand-green)" }), _jsx("circle", { cx: "29.5", cy: "22", r: "2", fill: "var(--color-brand-green)" })] }));
}
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
        }, children: _jsxs(GlassSurface, { elevated: true, className: "enter-fade-up", style: { width: 360, padding: "36px 32px 28px", borderRadius: "var(--panel-radius)" }, children: [_jsxs("div", { style: {
                        display: "flex",
                        flexDirection: "column",
                        alignItems: "center",
                        gap: 10,
                        marginBottom: 28,
                    }, children: [_jsx(BrandMark, {}), _jsxs("div", { style: { textAlign: "center" }, children: [_jsx("h1", { style: { fontSize: 20, fontWeight: 700, color: "var(--text-primary)", margin: 0 }, children: "\u6B22\u8FCE\u56DE\u6765" }), _jsx("p", { style: { fontSize: 13, color: "var(--text-secondary)", margin: "4px 0 0" }, children: "\u767B\u5F55\u4F60\u7684 MemoChat \u8D26\u53F7" })] })] }), _jsxs("form", { onSubmit: (e) => { void handleSubmit(e); }, style: { display: "flex", flexDirection: "column", gap: 14 }, children: [_jsx(GlassTextField, { label: "\u90AE\u7BB1", type: "email", value: email, onChange: (e) => setEmail(e.target.value), placeholder: "your@email.com", required: true, autoFocus: true }), _jsx(GlassTextField, { label: "\u5BC6\u7801", type: "password", value: password, onChange: (e) => setPassword(e.target.value), placeholder: "\u8BF7\u8F93\u5165\u5BC6\u7801", required: true }), _jsx("div", { style: { textAlign: "right", marginTop: -6 }, children: _jsx("button", { type: "button", onClick: () => navigate("/reset"), style: {
                                    background: "none",
                                    border: "none",
                                    color: "var(--text-secondary)",
                                    cursor: "pointer",
                                    fontSize: 12,
                                    padding: 0,
                                }, children: "\u5FD8\u8BB0\u5BC6\u7801\uFF1F" }) }), error && (_jsx("p", { style: {
                                color: "var(--color-badge)",
                                fontSize: 13,
                                margin: 0,
                                padding: "8px 10px",
                                borderRadius: 8,
                                background: "rgba(232,65,65,0.07)",
                                border: "1px solid rgba(232,65,65,0.16)",
                            }, children: error })), _jsx(GlassButton, { type: "submit", variant: "primary", disabled: loading, style: { marginTop: 4 }, children: loading ? _jsx(Spinner, { size: 18, color: "#fff" }) : "登录" })] }), _jsxs("div", { style: {
                        marginTop: 20,
                        paddingTop: 16,
                        borderTop: "1px solid var(--divider)",
                        textAlign: "center",
                        fontSize: 13,
                        color: "var(--text-secondary)",
                    }, children: ["\u8FD8\u6CA1\u6709\u8D26\u53F7\uFF1F", " ", _jsx("button", { onClick: () => navigate("/register"), style: {
                                background: "none",
                                border: "none",
                                color: "var(--color-brand-green)",
                                cursor: "pointer",
                                fontWeight: 500,
                                padding: 0,
                            }, children: "\u7ACB\u5373\u6CE8\u518C" })] })] }) }));
}
