/**
 * mediaUrl — mirrors IconPathUtils.
 * Normalizes media references without embedding credentials in URLs.
 */
import { runtimeConfig } from "@/core/config/runtimeConfig"

export const DEFAULT_AVATAR_DATA_URL = `data:image/svg+xml,${encodeURIComponent(`
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 96 96">
  <defs>
    <linearGradient id="g" x1="14" x2="82" y1="10" y2="88" gradientUnits="userSpaceOnUse">
      <stop stop-color="#7dd3fc"/>
      <stop offset=".56" stop-color="#a78bfa"/>
      <stop offset="1" stop-color="#f472b6"/>
    </linearGradient>
  </defs>
  <rect width="96" height="96" rx="48" fill="url(#g)"/>
  <circle cx="48" cy="35" r="17" fill="rgba(255,255,255,.86)"/>
  <path d="M19 78c5.6-16.4 16-25 29-25s23.4 8.6 29 25" fill="rgba(255,255,255,.78)"/>
</svg>
`)}`

function trimRef(ref: string): string {
  return ref.trim()
}

function isPassthroughUrl(ref: string): boolean {
  return ref.startsWith("data:") || ref.startsWith("blob:")
}

function isHttpUrl(ref: string): boolean {
  return ref.startsWith("http://") || ref.startsWith("https://")
}

function isDefaultAvatarRef(ref: string): boolean {
  const normalized = ref
    .replace(/^qrc:\//i, "")
    .replace(/^:\//, "")
    .replace(/^\//, "")
    .toLowerCase()
  return normalized === "res/head_1.png" ||
    normalized === "res/head_1.jpg" ||
    normalized === "res/head_1" ||
    normalized === "head_1" ||
    normalized === "head_1.png" ||
    normalized === "head_1.jpg" ||
    (/^head_\d+(?:\.(?:png|jpe?g))?$/.test(normalized) && !normalized.includes("/")) ||
    (/^res\/head_\d+(?:\.(?:png|jpe?g))?$/.test(normalized))
}

function looksLikeMediaKey(ref: string): boolean {
  // MediaService generates RFC 4122 v4 UUIDs. Matching the version and variant
  // prevents arbitrary dashed strings from becoming authenticated requests.
  return /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i.test(ref)
}

function mediaBaseUrl(): string {
  const configured =
    `${runtimeConfig.mediaBaseUrl ?? ""}`.trim() ||
    `${runtimeConfig.gateBaseUrl ?? ""}`.trim()
  return configured.replace(/\/+$/, "")
}

function urlParseBase(): string {
  return mediaBaseUrl() || "http://memochat.local"
}

function serializeUrl(parsed: URL, absolute: boolean): string {
  if (absolute) return parsed.toString()
  return `${parsed.pathname}${parsed.search}${parsed.hash}`
}

function isTrustedMediaEndpoint(ref: string, pathname: string): boolean {
  const parsed = new URL(ref, urlParseBase())
  if (parsed.pathname !== pathname) return false

  const hasExplicitOrigin = /^[a-z][a-z\d+.-]*:/i.test(ref) || ref.startsWith("//")
  if (!hasExplicitOrigin) return true

  const base = mediaBaseUrl()
  if (!base) return false
  try {
    return parsed.origin === new URL(base).origin
  } catch {
    return false
  }
}

function isLocalMediaHost(host: string): boolean {
  const normalized = host.trim().toLowerCase()
  return (
    normalized === "localhost" ||
    normalized === "127.0.0.1" ||
    normalized === "::1" ||
    normalized === "[::1]"
  )
}

function isMediaDownloadPath(pathname: string): boolean {
  if (pathname === "/media/download") return true
  if (!pathname.startsWith("/media/download/")) return false
  return !pathname.includes("..")
}

function isMediaDownloadUrl(ref: string): boolean {
  return isTrustedMediaEndpoint(ref, "/media/download")
}

function isR18ImageUrl(ref: string): boolean {
  return isTrustedMediaEndpoint(ref, "/api/r18/image")
}

function currentMediaDownloadUrl(ref: string): string {
  const source = new URL(ref, urlParseBase())
  const target = new URL("/media/download", urlParseBase())
  source.searchParams.forEach((value, key) => {
    if (key !== "uid" && key !== "token") target.searchParams.append(key, value)
  })
  return serializeUrl(target, Boolean(mediaBaseUrl()))
}

/** Rewrite legacy desktop absolute media URLs (often http://127.0.0.1:8080/...) */
function rewriteLocalMediaDownloadUrl(ref: string): string | null {
  try {
    const parsed = new URL(ref)
    if (!isMediaDownloadPath(parsed.pathname)) return null
    if (!isLocalMediaHost(parsed.hostname)) return null
    return currentMediaDownloadUrl(ref)
  } catch {
    return null
  }
}

function mediaDownloadUrl(mediaKey: string): string {
  const parsed = new URL("/media/download", urlParseBase())
  parsed.searchParams.set("asset", mediaKey)
  return currentMediaDownloadUrl(serializeUrl(parsed, Boolean(mediaBaseUrl())))
}

export function isAuthorizedMediaUrl(ref: string): boolean {
  if (!ref || isPassthroughUrl(ref)) return false
  return isMediaDownloadUrl(ref) || isR18ImageUrl(ref)
}

export function resolveMediaUrl(ref: string): string {
  const clean = trimRef(ref)
  if (!clean || isPassthroughUrl(clean)) return clean
  if (isDefaultAvatarRef(clean)) return DEFAULT_AVATAR_DATA_URL

  if (isHttpUrl(clean)) {
    const localRewrite = rewriteLocalMediaDownloadUrl(clean)
    if (localRewrite) return localRewrite
    return isMediaDownloadUrl(clean) ? currentMediaDownloadUrl(clean) : clean
  }

  if (clean.startsWith("media/download")) {
    return currentMediaDownloadUrl(clean)
  }
  if (clean.startsWith("/media/download")) {
    return currentMediaDownloadUrl(clean)
  }
  if (looksLikeMediaKey(clean)) {
    return mediaDownloadUrl(clean)
  }

  const path = clean.startsWith("/") ? clean : `/media/${clean.replace(/^media\//, "")}`
  const url = `${mediaBaseUrl()}${path}`
  return isMediaDownloadUrl(url) ? currentMediaDownloadUrl(url) : url
}

export function avatarUrl(icon: string | undefined | null): string {
  if (!icon) return DEFAULT_AVATAR_DATA_URL
  return resolveMediaUrl(icon)
}
