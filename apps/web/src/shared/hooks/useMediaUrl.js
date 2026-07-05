import { resolveMediaUrl } from "@/shared/media/mediaUrl";
import { useSessionStore } from "@/core/session/sessionStore";
/** Re-resolves media URLs when the session token changes */
export function useMediaUrl(ref) {
    // Subscribe to token to re-resolve when session changes
    useSessionStore((s) => s.token);
    return ref ? resolveMediaUrl(ref) : "";
}
