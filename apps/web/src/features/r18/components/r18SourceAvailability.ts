import type { R18ManagedAccount, R18Source } from "@/features/r18/api/r18Api"

export function isActionableSource(source: R18Source): boolean {
  if (source.id === "mock" || source.status === "staged-js") return false
  return Boolean(
    source.enabled
    || source.status === "auth-required"
    || source.status === "credentials-missing",
  )
}

export type AccountInteractionKind =
  | "required-account"
  | "optional-account"
  | "optional-cookie"
  | "optional-account-or-cookie"
  | "required-ehentai-auth"
  | "none"

/**
 * exhentai is members-only and bound to e-hentai accounts.
 * nhentai and hanime1 support both username/password and optional cookie login.
 * e-hentai supports optional cookie/password login.
 */
export function accountInteractionKind(account: R18ManagedAccount): AccountInteractionKind {
  if (account.source_id === "exhentai.official") return "required-ehentai-auth"
  if (account.source_id === "picacg.official" || account.auth_required) return "required-account"
  if (account.source_id === "jm.official") return "optional-account"
  if (account.source_id === "ehentai.official") return "optional-cookie"
  if (account.source_id === "nhentai.official") return "optional-account-or-cookie"
  if (account.source_id === "hanime1.official") return "optional-account-or-cookie"
  if (account.source_id === "hanimeone.official") return "optional-account-or-cookie"
  return "none"
}
