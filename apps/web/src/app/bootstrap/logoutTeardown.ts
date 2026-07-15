/**
 * logoutTeardown — mirrors ResetSession.
 * Clears session, entity store, and WS connection on logout.
 */
import { useSessionStore } from "@/core/session/sessionStore"
import { resetEntitySession } from "@/core/entities/resetSession"
import { getGateway, resetGateway } from "@/shared/gateway/ClientGateway"
import { FeatureRegistry } from "@/app/container/FeatureRegistry"
import { teardownContainer } from "@/app/container/DIContainer"
import { ENDPOINTS } from "@/core/config/endpoints"
import { logger } from "@/core/common/logger"
import { useChatStore } from "@/features/chat/store/chatStore"

export async function logoutTeardown(): Promise<void> {
  const session = useSessionStore.getState()
  if (session.token) {
    try {
      await getGateway().http.post(ENDPOINTS.authLogout, {
        client_ver: "3.0.0",
        all_devices: false,
      }, {
        headers: { "X-MemoChat-Client": "web" },
      })
    } catch (error) {
      logger.app.warn("Server logout failed; clearing the local session", error)
    }
  }
  FeatureRegistry.teardownAll()
  FeatureRegistry.clear()
  await resetEntitySession()
  // Chat UI selection/history flags are process-wide; clear them so the next
  // login does not reopen the previous account's peerId with empty entities.
  useChatStore.getState().reset()
  useSessionStore.getState().clearSession()
  teardownContainer()
  resetGateway()
}
