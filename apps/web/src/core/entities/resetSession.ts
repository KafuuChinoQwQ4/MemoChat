/**
 * resetSession — teardown for entity store + DB on logout.
 * Called by logoutTeardown.ts in app/bootstrap.
 */
import { useEntityStore } from "./entityStore"
import { clearUserMessages } from "./persistence/chatDb"
import { useSessionStore } from "@/core/session/sessionStore"
import { clearMediaAuthCache } from "@/shared/media/mediaAuthCache"

export async function resetEntitySession(): Promise<void> {
  const uid = useSessionStore.getState().uid
  useEntityStore.getState().reset()
  // Drop authenticated media blob URLs so the next account does not briefly
  // paint the previous user's avatars (and so we free the object URLs).
  clearMediaAuthCache()
  if (uid !== null) {
    await clearUserMessages(uid)
  }
}
