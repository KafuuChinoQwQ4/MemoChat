/** AuthGuard — redirects to /login if no session token */
import { Navigate, Outlet } from "react-router-dom"
import { useSessionStore } from "@/core/session/sessionStore"

export function AuthGuard() {
  const isLoggedIn = useSessionStore((s) => s.token !== null)
  if (!isLoggedIn) return <Navigate to="/login" replace />
  return <Outlet />
}
