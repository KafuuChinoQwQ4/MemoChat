/**
 * AppProviders — single composition root provider tree.
 * Order matters: Gateway first (infra), then Query, then Theme (needs settings store).
 * Feature providers are mounted in AppShell after login, not here.
 */
import type { ReactNode } from "react"
import { GatewayProvider } from "./GatewayProvider"
import { QueryProvider } from "./QueryProvider"
import { ThemeProvider } from "./ThemeProvider"
import { SessionRestoreBoot } from "@/app/bootstrap/SessionRestoreBoot"

export function AppProviders({ children }: { children: ReactNode }) {
  return (
    <GatewayProvider>
      <QueryProvider>
        <ThemeProvider>
          <SessionRestoreBoot>
            {children}
          </SessionRestoreBoot>
        </ThemeProvider>
      </QueryProvider>
    </GatewayProvider>
  )
}
