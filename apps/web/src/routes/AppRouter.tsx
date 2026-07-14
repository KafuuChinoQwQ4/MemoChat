/**
 * AppRouter — mirrors ShellViewModel::Page routing.
 * Public routes: /login /register /reset
 * Protected /app/* routes guarded by AuthGuard + BootstrapGate.
 */
import { lazy, Suspense } from "react"
import { createHashRouter, RouterProvider, Navigate } from "react-router-dom"
import { AuthGuard } from "./guards/AuthGuard"
import { BootstrapGate } from "./guards/BootstrapGate"
import { Spinner } from "@/shared/ui/primitives/Spinner"

// Lazy-load page bundles for code-splitting
const LoginPage    = lazy(() => import("@/features/auth/components/LoginPage").then((m) => ({ default: m.LoginPage })))
const RegisterPage = lazy(() => import("@/features/auth/components/RegisterPage").then((m) => ({ default: m.RegisterPage })))
const ResetPage    = lazy(() => import("@/features/auth/components/ResetPage").then((m) => ({ default: m.ResetPage })))
const AppShell     = lazy(() => import("./shell/AppShell").then((m) => ({ default: m.AppShell })))
const ChatTab      = lazy(() => import("@/features/chat/components/ChatShellContent").then((m) => ({ default: m.ChatShellContent })))
const ContactTab   = lazy(() => import("./shell/ContactsHubContent").then((m) => ({ default: m.ContactsHubContent })))
const MomentsTab   = lazy(() => import("@/features/moments/components/MomentsShellContent").then((m) => ({ default: m.MomentsShellContent })))
const R18Tab       = lazy(() => import("@/features/r18/components/R18ShellContent").then((m) => ({ default: m.R18ShellContent })))
const AgentTab     = lazy(() => import("@/features/agent/components/AgentShellContent").then((m) => ({ default: m.AgentShellContent })))
const SettingsTab  = lazy(() => import("@/features/settings/components/SettingsPane").then((m) => ({ default: m.SettingsPane })))

function PageFallback() {
  return (
    <div style={{ height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }}>
      <Spinner size={32} />
    </div>
  )
}

const router = createHashRouter([
  { path: "/", element: <Navigate to="/login" replace /> },
  { path: "/login",    element: <Suspense fallback={<PageFallback />}><LoginPage /></Suspense> },
  { path: "/register", element: <Suspense fallback={<PageFallback />}><RegisterPage /></Suspense> },
  { path: "/reset",    element: <Suspense fallback={<PageFallback />}><ResetPage /></Suspense> },
  {
    element: <AuthGuard />,
    children: [{
      element: <BootstrapGate />,
      children: [{
        path: "/app",
        element: <Suspense fallback={<PageFallback />}><AppShell /></Suspense>,
        children: [
          { index: true, element: <Navigate to="/app/chat" replace /> },
          { path: "chat",     element: <Suspense fallback={<PageFallback />}><ChatTab /></Suspense> },
          { path: "contacts", element: <Suspense fallback={<PageFallback />}><ContactTab /></Suspense> },
          { path: "moments",  element: <Suspense fallback={<PageFallback />}><MomentsTab /></Suspense> },
          { path: "r18",      element: <Suspense fallback={<PageFallback />}><R18Tab /></Suspense> },
          { path: "agent",    element: <Suspense fallback={<PageFallback />}><AgentTab /></Suspense> },
          { path: "settings", element: <Suspense fallback={<PageFallback />}><SettingsTab /></Suspense> },
        ],
      }],
    }],
  },
])

export function AppRouter() {
  return <RouterProvider router={router} />
}
