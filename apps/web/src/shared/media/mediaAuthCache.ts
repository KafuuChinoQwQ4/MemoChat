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

interface PendingCacheEntry {
  status: "pending"
  promise: Promise<string>
  controller: AbortController
  subscribers: number
}

type CacheEntry =
  | PendingCacheEntry
  | { status: "ready"; objectUrl: string }
  | { status: "error" }

const cache = new Map<string, CacheEntry>()
const MAX_CONCURRENT_MEDIA_FETCHES = 4
let activeFetches = 0
interface PendingFetch {
  run: () => void
  reject: (reason?: unknown) => void
  signal?: AbortSignal
  abortListener?: () => void
}

const pendingFetches: PendingFetch[] = []

function abortError(): DOMException {
  return new DOMException("The media request was aborted", "AbortError")
}

function runNextMediaFetch(): void {
  while (activeFetches < MAX_CONCURRENT_MEDIA_FETCHES) {
    const next = pendingFetches.shift()
    if (!next) return
    if (next.abortListener) next.signal?.removeEventListener("abort", next.abortListener)
    if (next.signal?.aborted) {
      next.reject(abortError())
      continue
    }
    next.run()
    return
  }
}

function scheduleMediaFetch<T>(task: () => Promise<T>, signal?: AbortSignal): Promise<T> {
  return new Promise<T>((resolve, reject) => {
    const run = () => {
      activeFetches += 1
      void task()
        .then(resolve, reject)
        .finally(() => {
          activeFetches -= 1
          runNextMediaFetch()
        })
    }

    if (signal?.aborted) {
      reject(abortError())
      return
    }
    if (activeFetches < MAX_CONCURRENT_MEDIA_FETCHES) {
      run()
    } else {
      const pending: PendingFetch = signal ? { run, reject, signal } : { run, reject }
      const abortListener = () => {
        const index = pendingFetches.indexOf(pending)
        if (index >= 0) pendingFetches.splice(index, 1)
        reject(abortError())
      }
      pending.abortListener = abortListener
      signal?.addEventListener("abort", abortListener, { once: true })
      pendingFetches.push(pending)
    }
  })
}

function cacheKey(resolvedUrl: string, token: string): string {
  return `${token}\0${resolvedUrl}`
}

function subscribeToPending(key: string, entry: PendingCacheEntry, signal?: AbortSignal): Promise<string> {
  if (signal?.aborted) return Promise.reject(abortError())

  entry.subscribers += 1
  return new Promise<string>((resolve, reject) => {
    let settled = false
    let abortListener: (() => void) | undefined
    const release = () => {
      if (settled) return
      settled = true
      if (abortListener) signal?.removeEventListener("abort", abortListener)
      entry.subscribers -= 1
    }

    abortListener = () => {
      release()
      if (entry.subscribers === 0 && cache.get(key) === entry) {
        cache.delete(key)
        entry.controller.abort()
      }
      reject(abortError())
    }
    signal?.addEventListener("abort", abortListener, { once: true })

    entry.promise.then(
      (objectUrl) => {
        if (settled) return
        release()
        resolve(objectUrl)
      },
      (error: unknown) => {
        if (settled) return
        release()
        reject(error)
      },
    )
  })
}

export function peekMediaAuthCache(resolvedUrl: string, token: string): string | null {
  const entry = cache.get(cacheKey(resolvedUrl, token))
  return entry?.status === "ready" ? entry.objectUrl : null
}

/**
 * Resolve an authorized media URL to a blob: object URL, sharing in-flight
 * fetches and caching the result for later remounts.
 */
export function loadMediaAuthUrl(resolvedUrl: string, token: string, signal?: AbortSignal): Promise<string> {
  const key = cacheKey(resolvedUrl, token)
  const existing = cache.get(key)
  if (existing?.status === "ready") return Promise.resolve(existing.objectUrl)
  if (existing?.status === "pending") return subscribeToPending(key, existing, signal)
  if (signal?.aborted) return Promise.reject(abortError())

  const controller = new AbortController()
  let entry: PendingCacheEntry
  const promise = scheduleMediaFetch(() => fetch(resolvedUrl, {
    headers: { Authorization: `Bearer ${token}` },
    credentials: "include",
    signal: controller.signal,
  }), controller.signal)
    .then((response) => {
      if (!response.ok) throw new Error(`media fetch failed: ${response.status}`)
      return response.blob()
    })
    .then((blob) => {
      if (controller.signal.aborted || cache.get(key) !== entry) throw abortError()
      const objectUrl = URL.createObjectURL(blob)
      cache.set(key, { status: "ready", objectUrl })
      return objectUrl
    })
    .catch((err: unknown) => {
      if (cache.get(key) === entry) {
        cache.set(key, { status: "error" })
      }
      throw err
    })

  entry = { status: "pending", promise, controller, subscribers: 0 }
  cache.set(key, entry)
  return subscribeToPending(key, entry, signal)
}

/** Drop all cached media URLs (logout / account switch). */
export function clearMediaAuthCache(): void {
  for (const entry of cache.values()) {
    if (entry.status === "pending") {
      entry.controller.abort()
    } else if (entry.status === "ready") {
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
