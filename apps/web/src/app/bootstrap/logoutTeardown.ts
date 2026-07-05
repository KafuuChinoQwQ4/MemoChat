/**
 * logoutTeardown — mirrors ResetSession.
 * Clears session, entity store, and WS connection on logout.
 */
import { useSessionStore } from "@/core/session/sessionStore"
import { resetEntitySession } from "@/core/entities/resetSession"
import { resetGateway } from "@/shared/gateway/ClientGateway"
import { FeatureRegistry } from "@/app/container/FeatureRegistry"
import { teardownContainer } from "@/app/container/DIContainer"

export async function logoutTeardown(): Promise<void> {
  FeatureRegistry.teardownAll()
  FeatureRegistry.clear()
  await resetEntitySession()
  useSessionStore.getState().clearSession()
  teardownContainer()
  resetGateway()
}
