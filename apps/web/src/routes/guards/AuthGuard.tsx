/** AuthGuard — redirects to /login after cookie restore if no access token exists */
import { Navigate, Outlet } from "react-router-dom"
import { useSessionStore } from "@/core/session/sessionStore"

export function AuthGuard() {
  const hasAccess = useSessionStore((s) => s.token !== null)
  if (!hasAccess) return <Navigate to="/login" replace />
  return <Outlet />
}
