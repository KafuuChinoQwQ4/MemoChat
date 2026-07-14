/**
 * SessionRestoreBoot — kicks off refresh-token restore on first paint.
 */
import { useEffect, useState, type ReactNode } from "react"
import { startSessionRestore } from "./sessionRestore"
import { Spinner } from "@/shared/ui/primitives/Spinner"

export function SessionRestoreBoot({ children }: { children: ReactNode }) {
  const [ready, setReady] = useState(false)

  useEffect(() => {
    let cancelled = false
    void startSessionRestore().finally(() => {
      if (!cancelled) setReady(true)
    })
    return () => {
      cancelled = true
    }
  }, [])

  if (!ready) {
    return (
      <div style={{
        height: "100%",
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        gap: 12,
        color: "var(--text-secondary)",
        fontSize: 14,
      }}>
        <Spinner size={28} />
        <span>正在恢复登录…</span>
      </div>
    )
  }

  return <>{children}</>
}
