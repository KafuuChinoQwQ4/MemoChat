import { describe, expect, it } from "vitest"
import {
  accountInteractionKind,
  isActionableSource,
  type AccountInteractionKind,
} from "./r18SourceAvailability"

describe("R18 source availability", () => {
  it("keeps login-required sources actionable and removes unavailable entries", () => {
    expect(isActionableSource({ id: "picacg.official", enabled: false, status: "auth-required" })).toBe(true)
    expect(isActionableSource({ id: "nhentai.official", enabled: true, status: "ok" })).toBe(true)
    expect(isActionableSource({ id: "staged", enabled: false, status: "staged-js" })).toBe(false)
    expect(isActionableSource({ id: "mock", enabled: true, status: "ok" })).toBe(false)
  })

  it.each<[string, AccountInteractionKind]>([
    ["picacg.official", "required-account"],
    ["jm.official", "optional-account"],
    ["ehentai.official", "optional-cookie"],
    ["nhentai.official", "none"],
  ])("maps %s to an actionable account mode", (sourceId, expected) => {
    expect(accountInteractionKind({ source_id: sourceId, auth_required: sourceId === "picacg.official" })).toBe(expected)
  })
})
