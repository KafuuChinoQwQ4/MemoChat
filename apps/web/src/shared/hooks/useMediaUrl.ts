import { useEffect, useMemo, useState } from "react"
import { isAuthorizedMediaUrl, resolveMediaUrl } from "@/shared/media/mediaUrl"
import { loadMediaAuthUrl, peekMediaAuthCache } from "@/shared/media/mediaAuthCache"
import { useSessionStore } from "@/core/session/sessionStore"

/** Resolves media URLs and fetches protected media with Authorization headers. */
export function useMediaUrl(ref: string | undefined | null): string {
  const token = useSessionStore((s) => s.token)
  const resolved = useMemo(() => (ref ? resolveMediaUrl(ref) : ""), [ref])

  // Seed from the process-wide cache so remounts (dialog switch) paint instantly
  // without a blank frame while a new fetch would otherwise start.
  const [url, setUrl] = useState(() => {
    if (!resolved) return ""
    if (!isAuthorizedMediaUrl(resolved)) return resolved
    if (!token) return ""
    return peekMediaAuthCache(resolved, token) ?? ""
  })

  useEffect(() => {
    if (!resolved) {
      setUrl("")
      return
    }
    if (!isAuthorizedMediaUrl(resolved)) {
      setUrl(resolved)
      return
    }
    if (!token || typeof URL.createObjectURL !== "function") {
      setUrl("")
      return
    }

    const cached = peekMediaAuthCache(resolved, token)
    if (cached) {
      setUrl(cached)
      return
    }

    let cancelled = false
    // Keep previous paint if we already have a URL for this ref; otherwise blank
    // until the fetch lands (first mount only).
    setUrl((prev) => prev)
    void loadMediaAuthUrl(resolved, token)
      .then((objectUrl) => {
        if (!cancelled && objectUrl) setUrl(objectUrl)
      })
      .catch(() => {
        if (!cancelled) setUrl("")
      })

    return () => {
      cancelled = true
      // Do NOT revoke the blob here — the shared cache owns its lifetime so
      // switching conversations can reuse the same object URL.
    }
  }, [resolved, token])

  return url
}
