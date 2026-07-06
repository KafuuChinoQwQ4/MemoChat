import { jsx as _jsx } from "react/jsx-runtime";
/**
 * AppRouter — mirrors ShellViewModel::Page routing.
 * Public routes: /login /register /reset
 * Protected /app/* routes guarded by AuthGuard + BootstrapGate.
 */
import { lazy, Suspense } from "react";
import { createHashRouter, RouterProvider, Navigate } from "react-router-dom";
import { AuthGuard } from "./guards/AuthGuard";
import { BootstrapGate } from "./guards/BootstrapGate";
import { Spinner } from "@/shared/ui/primitives/Spinner";
// Lazy-load page bundles for code-splitting
const LoginPage = lazy(() => import("@/features/auth/components/LoginPage").then((m) => ({ default: m.LoginPage })));
const RegisterPage = lazy(() => import("@/features/auth/components/RegisterPage").then((m) => ({ default: m.RegisterPage })));
const ResetPage = lazy(() => import("@/features/auth/components/ResetPage").then((m) => ({ default: m.ResetPage })));
const AppShell = lazy(() => import("./shell/AppShell").then((m) => ({ default: m.AppShell })));
const ChatTab = lazy(() => import("@/features/chat/components/ChatShellContent").then((m) => ({ default: m.ChatShellContent })));
const ContactTab = lazy(() => import("@/features/contact/components/ContactShellContent").then((m) => ({ default: m.ContactShellContent })));
const GroupTab = lazy(() => import("@/features/group/components/GroupShellContent").then((m) => ({ default: m.GroupShellContent })));
const MomentsTab = lazy(() => import("@/features/moments/components/MomentsShellContent").then((m) => ({ default: m.MomentsShellContent })));
const R18Tab = lazy(() => import("@/features/r18/components/R18ShellContent").then((m) => ({ default: m.R18ShellContent })));
const AgentTab = lazy(() => import("@/features/agent/components/AgentShellContent").then((m) => ({ default: m.AgentShellContent })));
const SettingsTab = lazy(() => import("@/features/settings/components/SettingsPane").then((m) => ({ default: m.SettingsPane })));
function PageFallback() {
    return (_jsx("div", { style: { height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }, children: _jsx(Spinner, { size: 32 }) }));
}
const router = createHashRouter([
    { path: "/", element: _jsx(Navigate, { to: "/login", replace: true }) },
    { path: "/login", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(LoginPage, {}) }) },
    { path: "/register", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(RegisterPage, {}) }) },
    { path: "/reset", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(ResetPage, {}) }) },
    {
        element: _jsx(AuthGuard, {}),
        children: [{
                element: _jsx(BootstrapGate, {}),
                children: [{
                        path: "/app",
                        element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(AppShell, {}) }),
                        children: [
                            { index: true, element: _jsx(Navigate, { to: "/app/chat", replace: true }) },
                            { path: "chat", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(ChatTab, {}) }) },
                            { path: "contacts", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(ContactTab, {}) }) },
                            { path: "groups", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(GroupTab, {}) }) },
                            { path: "moments", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(MomentsTab, {}) }) },
                            { path: "r18", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(R18Tab, {}) }) },
                            { path: "agent", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(AgentTab, {}) }) },
                            { path: "settings", element: _jsx(Suspense, { fallback: _jsx(PageFallback, {}), children: _jsx(SettingsTab, {}) }) },
                        ],
                    }],
            }],
    },
]);
export function AppRouter() {
    return _jsx(RouterProvider, { router: router });
}
