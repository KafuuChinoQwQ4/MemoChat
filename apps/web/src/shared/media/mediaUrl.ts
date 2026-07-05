/**
 * mediaUrl — mirrors IconPathUtils.
 * Appends auth token as query param for media downloads.
 * NOTE: token in query may be logged; future hardening = header/cookie auth.
 */
import { runtimeConfig } from "@/core/config/runtimeConfig"
import { useSessionStore } from "@/core/session/sessionStore"

export function resolveMediaUrl(ref: string): string {
  if (!ref || ref.startsWith("data:") || ref.startsWith("blob:")) return ref
  if (ref.startsWith("http://") || ref.startsWith("https://")) return ref
  const token = useSessionStore.getState().token
  const base = runtimeConfig.mediaBaseUrl
  const sep = ref.includes("?") ? "&" : "?"
  return token ? `${base}/media/${ref}${sep}token=${encodeURIComponent(token)}` : `${base}/media/${ref}`
}

export function avatarUrl(icon: string | undefined | null): string {
  if (!icon) return ""
  return resolveMediaUrl(icon)
}
