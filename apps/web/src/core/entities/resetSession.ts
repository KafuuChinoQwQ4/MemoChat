/**
 * resetSession — teardown for entity store + DB on logout.
 * Called by logoutTeardown.ts in app/bootstrap.
 */
import { useEntityStore } from "./entityStore"
import { clearUserMessages } from "./persistence/chatDb"
import { useSessionStore } from "@/core/session/sessionStore"

export async function resetEntitySession(): Promise<void> {
  const uid = useSessionStore.getState().uid
  useEntityStore.getState().reset()
  if (uid !== null) {
    await clearUserMessages(uid)
  }
}
