/**
 * sessionRestore — re-issue the access session from the HttpOnly refresh cookie.
 * Runs once on app boot before protected routes render content.
 */
import { useSessionStore } from "@/core/session/sessionStore"
import { logger } from "@/core/common/logger"
import { restoreSessionFromRefresh } from "./postLoginBootstrap"

let restorePromise: Promise<boolean> | null = null

export function startSessionRestore(): Promise<boolean> {
  if (restorePromise) return restorePromise

  restorePromise = (async () => {
    const session = useSessionStore.getState()
    if (session.token) return true

    session.setConnState("reconnecting")
    try {
      await restoreSessionFromRefresh()
      return true
    } catch (error) {
      logger.app.warn("Session restore failed, clearing in-memory auth", error)
      useSessionStore.getState().clearSession()
      return false
    }
  })()

  return restorePromise
}
