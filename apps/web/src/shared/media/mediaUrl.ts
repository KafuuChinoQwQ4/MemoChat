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
  return ref.length >= 16 &&
    ref.length <= 96 &&
    ref.includes("-") &&
    /^[A-Za-z0-9_-]+$/.test(ref)
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

function shouldReturnAbsolute(originalUrl: string): boolean {
  return isHttpUrl(originalUrl) || Boolean(mediaBaseUrl())
}

function serializeUrl(parsed: URL, absolute: boolean): string {
  if (absolute) return parsed.toString()
  return `${parsed.pathname}${parsed.search}${parsed.hash}`
}

function isMediaDownloadUrl(ref: string): boolean {
  const parsed = new URL(ref, urlParseBase())
  return parsed.pathname === "/media/download"
}

function isR18ImageUrl(ref: string): boolean {
  const parsed = new URL(ref, urlParseBase())
  return parsed.pathname === "/api/r18/image"
}

function currentMediaDownloadUrl(ref: string): string {
  const source = new URL(ref, urlParseBase())
  const target = new URL("/media/download", urlParseBase())
  source.searchParams.forEach((value, key) => {
    if (key !== "uid" && key !== "token") target.searchParams.append(key, value)
  })
  return serializeUrl(target, Boolean(mediaBaseUrl()))
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

export function absoluteMediaUrl(ref: string): string {
  return new URL(ref, urlParseBase()).toString()
}

export function resolveMediaUrl(ref: string): string {
  const clean = trimRef(ref)
  if (!clean || isPassthroughUrl(clean)) return clean
  if (isDefaultAvatarRef(clean)) return DEFAULT_AVATAR_DATA_URL

  if (isHttpUrl(clean)) {
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
