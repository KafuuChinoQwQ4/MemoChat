import { beforeEach, describe, expect, it } from "vitest"
import { useSessionStore } from "@/core/session/sessionStore"
import { avatarUrl, isAuthorizedMediaUrl, resolveMediaUrl } from "./mediaUrl"

describe("media URL resolution", () => {
  beforeEach(() => {
    useSessionStore.getState().clearSession()
    useSessionStore.getState().setLogin({
      uid: 42,
      token: "tok value",
      loginTicket: "ticket",
      ticketExpireMs: Date.now() + 60_000,
      protocolVersion: 3,
      chatEndpoints: [],
      profile: { uid: 42, name: "tester", email: "t@example.test", icon: "" },
    })
  })

  it("keeps media download paths on /media/download without embedding credentials", () => {
    expect(resolveMediaUrl("media/download?asset=avatar-key")).toBe(
      "/media/download?asset=avatar-key",
    )
    expect(resolveMediaUrl("/media/download?asset=avatar-key")).toBe(
      "/media/download?asset=avatar-key",
    )
  })

  it("rewrites local legacy absolute media download URLs to same-origin authorized paths", () => {
    const legacy = "http://127.0.0.1:8080/media/download?asset=avatar-key&token=stale"
    expect(resolveMediaUrl(legacy)).toBe("/media/download?asset=avatar-key")
    expect(isAuthorizedMediaUrl(resolveMediaUrl(legacy))).toBe(true)
    expect(resolveMediaUrl("http://localhost:8094/media/download?asset=avatar-key")).toBe(
      "/media/download?asset=avatar-key",
    )
  })

  it("does not rewrite or authorize an absolute download URL from another origin", () => {
    const external = "http://evil.example/media/download?asset=avatar-key"
    expect(resolveMediaUrl(external)).toBe(external)
    expect(isAuthorizedMediaUrl(external)).toBe(false)
  })

  it("turns generated UUID v4 media keys into canonical download URLs", () => {
    expect(resolveMediaUrl("4c06df5e-2f26-4de1-8c24-3d03ea5d1be7")).toBe(
      "/media/download?asset=4c06df5e-2f26-4de1-8c24-3d03ea5d1be7",
    )
  })

  it("does not treat arbitrary dashed strings or non-v4 UUIDs as media keys", () => {
    expect(resolveMediaUrl("abc-def_1234567890")).toBe("/media/abc-def_1234567890")
    expect(resolveMediaUrl("4c06df5e-2f26-1de1-8c24-3d03ea5d1be7")).toBe(
      "/media/4c06df5e-2f26-1de1-8c24-3d03ea5d1be7",
    )
  })

  it("never authorizes lookalike media endpoints on an external origin", () => {
    expect(isAuthorizedMediaUrl("https://attacker.example/media/download?asset=stolen")).toBe(false)
    expect(isAuthorizedMediaUrl("https://attacker.example/api/r18/image?image_url=x")).toBe(false)
    expect(isAuthorizedMediaUrl("/media/download?asset=owned")).toBe(true)
  })

  it("keeps non-download media paths same-origin when no media base URL is configured", () => {
    expect(resolveMediaUrl("avatars/a.png")).toBe("/media/avatars/a.png")
    expect(resolveMediaUrl("/media/avatars/a.png")).toBe("/media/avatars/a.png")
  })

  it("maps QML default avatar refs to a browser-safe data image", () => {
    expect(avatarUrl("")).toMatch(/^data:image\/svg\+xml,/)
    expect(avatarUrl(null)).toMatch(/^data:image\/svg\+xml,/)
    expect(avatarUrl(":/res/head_1.png")).toMatch(/^data:image\/svg\+xml,/)
    expect(avatarUrl("qrc:/res/head_1.png")).toMatch(/^data:image\/svg\+xml,/)
    expect(avatarUrl("head_1")).toMatch(/^data:image\/svg\+xml,/)
    expect(avatarUrl("head_1.jpg")).toMatch(/^data:image\/svg\+xml,/)
  })
})
