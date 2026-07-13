import type { R18ManagedAccount, R18Source } from "@/features/r18/api/r18Api"

export function isActionableSource(source: R18Source): boolean {
  if (source.id === "mock" || source.status === "staged-js") return false
  return Boolean(
    source.enabled
    || source.status === "auth-required"
    || source.status === "credentials-missing",
  )
}

export type AccountInteractionKind = "required-account" | "optional-account" | "optional-cookie" | "none"

export function accountInteractionKind(account: R18ManagedAccount): AccountInteractionKind {
  if (account.source_id === "picacg.official" || account.auth_required) return "required-account"
  if (account.source_id === "jm.official") return "optional-account"
  if (account.source_id === "ehentai.official") return "optional-cookie"
  return "none"
}
