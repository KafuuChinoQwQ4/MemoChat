/**
 * Process-wide cache for authenticated media (avatars, chat images, etc.).
 *
 * Protected media is fetched with Authorization and rendered via blob: URLs.
 * Without a shared cache, every Avatar remount (dialog switch, list re-render)
 * re-fetches MinIO-backed assets and immediately revokes the previous blob —
 * causing the "reload on every conversation switch" flicker.
 *
 * Ready entries are retained across remounts until clearMediaAuthCache() runs
 * (logout / account switch). In-flight fetches are shared by key.
 */

type CacheEntry =
  | { status: "pending"; promise: Promise<string> }
  | { status: "ready"; objectUrl: string }
  | { status: "error" }

const cache = new Map<string, CacheEntry>()

function cacheKey(resolvedUrl: string, token: string): string {
  return `${token}\0${resolvedUrl}`
}

export function peekMediaAuthCache(resolvedUrl: string, token: string): string | null {
  const entry = cache.get(cacheKey(resolvedUrl, token))
  return entry?.status === "ready" ? entry.objectUrl : null
}

/**
 * Resolve an authorized media URL to a blob: object URL, sharing in-flight
 * fetches and caching the result for later remounts.
 */
export function loadMediaAuthUrl(resolvedUrl: string, token: string): Promise<string> {
  const key = cacheKey(resolvedUrl, token)
  const existing = cache.get(key)
  if (existing?.status === "ready") return Promise.resolve(existing.objectUrl)
  if (existing?.status === "pending") return existing.promise

  const promise = fetch(resolvedUrl, {
    headers: { Authorization: `Bearer ${token}` },
  })
    .then((response) => {
      if (!response.ok) throw new Error(`media fetch failed: ${response.status}`)
      return response.blob()
    })
    .then((blob) => {
      // If the cache was cleared (logout) while in flight, still return a URL
      // for the waiter but do not re-insert into the cache.
      if (!cache.has(key)) {
        return URL.createObjectURL(blob)
      }
      const objectUrl = URL.createObjectURL(blob)
      cache.set(key, { status: "ready", objectUrl })
      return objectUrl
    })
    .catch((err: unknown) => {
      if (cache.get(key)?.status === "pending") {
        cache.set(key, { status: "error" })
      }
      throw err
    })

  cache.set(key, { status: "pending", promise })
  return promise
}

/** Drop all cached media URLs (logout / account switch). */
export function clearMediaAuthCache(): void {
  for (const entry of cache.values()) {
    if (entry.status === "ready") {
      try {
        URL.revokeObjectURL(entry.objectUrl)
      } catch {
        // ignore revoke races
      }
    }
  }
  cache.clear()
}

/** Test helper — current number of cache entries. */
export function mediaAuthCacheSizeForTests(): number {
  return cache.size
}
