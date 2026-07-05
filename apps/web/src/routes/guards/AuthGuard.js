import { jsx as _jsx } from "react/jsx-runtime";
/** AuthGuard — redirects to /login if no session token */
import { Navigate, Outlet } from "react-router-dom";
import { useSessionStore } from "@/core/session/sessionStore";
export function AuthGuard() {
    const isLoggedIn = useSessionStore((s) => s.token !== null);
    if (!isLoggedIn)
        return _jsx(Navigate, { to: "/login", replace: true });
    return _jsx(Outlet, {});
}
