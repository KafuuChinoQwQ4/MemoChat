import { beforeEach, describe, expect, it, vi } from "vitest"
import { useSessionStore } from "@/core/session/sessionStore"
import { restoreSessionFromRefresh } from "./postLoginBootstrap"
import { startSessionRestore } from "./sessionRestore"

vi.mock("./postLoginBootstrap", () => ({
  restoreSessionFromRefresh: vi.fn(),
}))

describe("startSessionRestore", () => {
  beforeEach(() => {
    vi.mocked(restoreSessionFromRefresh).mockReset()
    useSessionStore.getState().clearSession()
  })

  it("attempts cookie-backed refresh without a JavaScript refresh token", async () => {
    vi.mocked(restoreSessionFromRefresh).mockResolvedValue(undefined)

    await expect(startSessionRestore()).resolves.toBe(true)
    expect(restoreSessionFromRefresh).toHaveBeenCalledOnce()
    expect(restoreSessionFromRefresh).toHaveBeenCalledWith()
  })
})
