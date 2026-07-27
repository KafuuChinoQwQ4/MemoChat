import { useCallback, useEffect, useMemo, useState } from "react"
import { isAuthorizedMediaUrl, resolveMediaUrl } from "@/shared/media/mediaUrl"
import { loadMediaAuthUrl, peekMediaAuthCache } from "@/shared/media/mediaAuthCache"
import { useSessionStore } from "@/core/session/sessionStore"

export interface MediaUrlState {
  url: string
  isLoading: boolean
  error: string | null
  retry: () => void
}

/** Resolves media URLs and exposes a terminal state for protected media failures. */
export function useMediaUrlState(ref: string | undefined | null): MediaUrlState {
  const token = useSessionStore((s) => s.token)
  const resolved = useMemo(() => (ref ? resolveMediaUrl(ref) : ""), [ref])
  const [attempt, setAttempt] = useState(0)

  // Seed from the process-wide cache so remounts (dialog switch) paint instantly
  // without a blank frame while a new fetch would otherwise start.
  const [url, setUrl] = useState(() => {
    if (!resolved) return ""
    if (!isAuthorizedMediaUrl(resolved)) return resolved
    if (!token) return ""
    return peekMediaAuthCache(resolved, token) ?? ""
  })
  const [isLoading, setIsLoading] = useState(false)
  const [error, setError] = useState<string | null>(null)

  const retry = useCallback(() => {
    setAttempt((value) => value + 1)
  }, [])

  useEffect(() => {
    if (!resolved) {
      setUrl("")
      setIsLoading(false)
      setError(null)
      return
    }
    if (!isAuthorizedMediaUrl(resolved)) {
      setUrl(resolved)
      setIsLoading(false)
      setError(null)
      return
    }
    if (!token || typeof URL.createObjectURL !== "function") {
      setUrl("")
      setIsLoading(false)
      setError(token ? "当前环境无法显示图片" : "登录状态已失效")
      return
    }

    const cached = peekMediaAuthCache(resolved, token)
    if (cached) {
      setUrl(cached)
      setIsLoading(false)
      setError(null)
      return
    }

    let cancelled = false
    const controller = new AbortController()
    setUrl("")
    setIsLoading(true)
    setError(null)
    void loadMediaAuthUrl(resolved, token, controller.signal)
      .then((objectUrl) => {
        if (!cancelled && objectUrl) {
          setUrl(objectUrl)
          setIsLoading(false)
        }
      })
      .catch((reason: unknown) => {
        if (!cancelled) {
          setUrl("")
          setIsLoading(false)
          setError(reason instanceof Error ? reason.message : "图片加载失败")
        }
      })

    return () => {
      cancelled = true
      controller.abort()
      // Do NOT revoke the blob here — the shared cache owns its lifetime so
      // switching conversations can reuse the same object URL.
    }
  }, [attempt, resolved, token])

  return { url, isLoading, error, retry }
}

/** Resolves media URLs and fetches protected media with Authorization headers. */
export function useMediaUrl(ref: string | undefined | null): string {
  return useMediaUrlState(ref).url
}
