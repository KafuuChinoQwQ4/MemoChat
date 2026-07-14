import { describe, expect, it, vi } from "vitest"
import type { HttpClient } from "@/core/network/http/HttpClient"
import { createR18Api } from "./r18Api"

function mockHttp() {
  return {
    get: vi.fn(),
    post: vi.fn(),
  }
}

describe("createR18Api", () => {
  it("uses the server-owned access endpoints", async () => {
    const http = mockHttp()
    http.get.mockResolvedValue({
      error: 0,
      data: { allowed: false, adult_attested_at_ms: 0, state: "denied", can_attest: true },
    })
    http.post.mockResolvedValue({
      error: 0,
      data: { allowed: true, adult_attested_at_ms: 123, state: "allowed", can_attest: true },
    })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(api.getAccess()).resolves.toMatchObject({ allowed: false, state: "denied" })
    await expect(api.attestAdult()).resolves.toMatchObject({ allowed: true, state: "allowed" })
    expect(http.get).toHaveBeenCalledWith("/api/r18/access")
    expect(http.post).toHaveBeenCalledWith("/api/r18/access/attest", {})
  })

  it("matches comic detail and chapter page response shapes", async () => {
    const http = mockHttp()
    http.post
      .mockResolvedValueOnce({ error: 0, data: { chapters: [{ chapter_id: "chapter-1", title: "One" }] } })
      .mockResolvedValueOnce({
        error: 0,
        data: { pages: [{ url: "/api/r18/image?image_id=1" }, { image_id: "missing-url" }, { url: "  " }] },
      })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(api.listChapters("source", "comic")).resolves.toEqual([
      { chapter_id: "chapter-1", title: "One" },
    ])
    await expect(api.listPageUrls("source", "chapter-1")).resolves.toEqual([
      "/api/r18/image?image_id=1",
    ])
    expect(http.post).toHaveBeenNthCalledWith(1, "/api/r18/comic/detail", {
      source_id: "source",
      comic_id: "comic",
    })
    expect(http.post).toHaveBeenNthCalledWith(2, "/api/r18/chapter/pages", {
      source_id: "source",
      chapter_id: "chapter-1",
    })
  })

  it("fails closed on an application error envelope", async () => {
    const http = mockHttp()
    http.get.mockResolvedValue({ error: 1002, message: "policy unavailable" })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(api.getAccess()).rejects.toThrow("policy unavailable")
  })

  it("uses account management endpoints", async () => {
    const http = mockHttp()
    http.get.mockResolvedValue({
      error: 0,
      data: {
        managed: [{ source_id: "picacg.official", status: "not_configured", auth_required: true }],
      },
    })
    http.post
      .mockResolvedValueOnce({
        error: 0,
        data: { managed: [{ source_id: "picacg.official", status: "authenticated", username: "a@b.c" }] },
      })
      .mockResolvedValueOnce({
        error: 0,
        data: { managed: [{ source_id: "picacg.official", status: "not_configured" }] },
      })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(api.listAccounts()).resolves.toMatchObject({
      managed: [{ source_id: "picacg.official" }],
    })
    await expect(api.saveAccount("picacg.official", "a@b.c", "secret")).resolves.toMatchObject({
      managed: [{ status: "authenticated" }],
    })
    await expect(api.clearAccount("picacg.official")).resolves.toMatchObject({
      managed: [{ status: "not_configured" }],
    })
    expect(http.get).toHaveBeenCalledWith("/api/r18/accounts")
    expect(http.post).toHaveBeenNthCalledWith(1, "/api/r18/account/save", {
      source_id: "picacg.official",
      username: "a@b.c",
      password: "secret",
    })
    expect(http.post).toHaveBeenNthCalledWith(2, "/api/r18/account/clear", {
      source_id: "picacg.official",
    })
  })

  it("forwards sort/tag filter options on search", async () => {
    const http = mockHttp()
    http.post.mockResolvedValue({
      error: 0,
      data: { items: [], max_page: 1, sort: "mv_t", tag: "同人" },
    })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(
      api.search("jm.official", "foo", 2, { sort: "mv_t", tag: "同人" }),
    ).resolves.toMatchObject({ sort: "mv_t", tag: "同人" })
    expect(http.post).toHaveBeenCalledWith("/api/r18/search", {
      source_id: "jm.official",
      keyword: "foo",
      page: 2,
      sort: "mv_t",
      tag: "同人",
    })
  })
})
