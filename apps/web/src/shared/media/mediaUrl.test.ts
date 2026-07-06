import { beforeEach, describe, expect, it } from "vitest"
import { useSessionStore } from "@/core/session/sessionStore"
import { avatarUrl, resolveMediaUrl } from "./mediaUrl"

describe("media URL resolution", () => {
  beforeEach(() => {
    useSessionStore.getState().clearSession()
    useSessionStore.getState().setLogin({
      uid: 42,
      token: "tok value",
      loginTicket: "ticket",
      ticketExpireMs: Date.now() + 60_000,
      refreshToken: "refresh",
      protocolVersion: 3,
      chatEndpoints: [],
      profile: { uid: 42, name: "tester", email: "t@example.test", icon: "" },
    })
  })

  it("keeps media download paths on /media/download and appends uid plus token", () => {
    expect(resolveMediaUrl("media/download?asset=avatar-key")).toBe(
      "/media/download?asset=avatar-key&uid=42&token=tok+value",
    )
    expect(resolveMediaUrl("/media/download?asset=avatar-key")).toBe(
      "/media/download?asset=avatar-key&uid=42&token=tok+value",
    )
  })

  it("normalizes legacy localhost media download URLs to the current gateway path", () => {
    expect(resolveMediaUrl("http://127.0.0.1:8080/media/download?asset=avatar-key")).toBe(
      "/media/download?asset=avatar-key&uid=42&token=tok+value",
    )
  })

  it("turns bare media keys into authenticated download URLs", () => {
    expect(resolveMediaUrl("abc-def_1234567890")).toBe(
      "/media/download?asset=abc-def_1234567890&uid=42&token=tok+value",
    )
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
