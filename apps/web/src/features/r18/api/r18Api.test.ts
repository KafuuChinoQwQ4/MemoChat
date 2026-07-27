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

  it("resolves a fresh Hanime1 playback descriptor with an abort signal", async () => {
    const http = mockHttp()
    const controller = new AbortController()
    http.post.mockResolvedValue({
      error: 0,
      data: {
        source_id: "hanime1.official",
        chapter_id: "407339",
        poster: "/api/r18/image?source_id=hanime1.official&image_url=poster",
        expires_at_ms: 1_785_121_222_000,
        sources: [
          {
            url: "https://vdownload.hembed.com/407339-1080p.mp4?secure=test,1785121222",
            mime_type: "video/mp4",
            quality: 1080,
          },
        ],
      },
    })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(
      api.resolveVideo("hanime1.official", "407339", controller.signal),
    ).resolves.toMatchObject({
      source_id: "hanime1.official",
      chapter_id: "407339",
      expires_at_ms: 1_785_121_222_000,
      sources: [{ mime_type: "video/mp4", quality: 1080 }],
    })
    expect(http.post).toHaveBeenCalledWith(
      "/api/r18/video/resolve",
      { source_id: "hanime1.official", chapter_id: "407339" },
      { signal: controller.signal },
    )
  })

  it("rejects a comic detail failure instead of presenting an empty chapter", async () => {
    const http = mockHttp()
    http.post.mockResolvedValue({
      error: 0,
      data: {
        title: "官方源请求失败",
        description: "upstream detail failed",
        chapters: [],
      },
    })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(api.listChapters("source", "comic")).rejects.toThrow("upstream detail failed")
  })

  it("rejects a chapter page failure instead of presenting an empty chapter", async () => {
    const http = mockHttp()
    http.post.mockResolvedValue({
      error: 0,
      data: { error_message: "upstream pages failed", pages: [] },
    })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(api.listPageUrls("source", "chapter-1")).rejects.toThrow("upstream pages failed")
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

  it("startBrowserImport posts to the correct endpoint with web_extension client kind", async () => {
    const http = mockHttp()
    http.post.mockResolvedValue({
      error: 0,
      data: { import_id: "imp_abc", ticket: "tkt_xyz", expires_at_ms: 9999 },
    })
    const api = createR18Api(http as unknown as HttpClient)

    const result = await api.startBrowserImport("exhentai.official")
    expect(result).toEqual({ import_id: "imp_abc", ticket: "tkt_xyz", expires_at_ms: 9999 })
    expect(http.post).toHaveBeenCalledWith(
      "/api/r18/account/browser-import/start",
      { source_id: "exhentai.official", client_kind: "web_extension" },
    )
  })

  it("getBrowserImportStatus encodes the import_id query param", async () => {
    const http = mockHttp()
    http.get.mockResolvedValue({
      error: 0,
      data: { status: "pending", message: "" },
    })
    const api = createR18Api(http as unknown as HttpClient)

    const result = await api.getBrowserImportStatus("imp_test/id")
    expect(result).toEqual({ status: "pending", message: "" })
    expect(http.get).toHaveBeenCalledWith(
      "/api/r18/account/browser-import/status?import_id=imp_test%2Fid",
    )
  })

  it("getBrowserImportStatus propagates authenticated status", async () => {
    const http = mockHttp()
    http.get.mockResolvedValue({
      error: 0,
      data: { status: "authenticated", message: "Session imported" },
    })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(api.getBrowserImportStatus("imp_ok")).resolves.toMatchObject({
      status: "authenticated",
      message: "Session imported",
    })
  })

  it("importSession sends cookies as a nested object with source_id", async () => {
    const http = mockHttp()
    http.post.mockResolvedValue({
      error: 0,
      data: { success: true, message: "ok", ehentai_access: true, exhentai_access: false },
    })
    const api = createR18Api(http as unknown as HttpClient)

    const result = await api.importSession({
      sourceId: "ehentai.official",
      ipb_member_id: "1234",
      ipb_pass_hash: "abcdef",
      igneous: "",
      sk: "",
    })
    expect(result).toMatchObject({ success: true, ehentai_access: true })
    expect(http.post).toHaveBeenCalledWith(
      "/api/r18/account/session/import",
      {
        source_id: "ehentai.official",
        cookies: {
          ipb_member_id: "1234",
          ipb_pass_hash: "abcdef",
          igneous: "",
          sk: "",
        },
      },
    )
  })

  it("importSession defaults igneous and sk to empty string when omitted", async () => {
    const http = mockHttp()
    http.post.mockResolvedValue({ error: 0, data: { success: true } })
    const api = createR18Api(http as unknown as HttpClient)

    await api.importSession({ sourceId: "ehentai.official", ipb_member_id: "1", ipb_pass_hash: "2" })
    expect(http.post).toHaveBeenCalledWith(
      "/api/r18/account/session/import",
      {
        source_id: "ehentai.official",
        cookies: {
          ipb_member_id: "1",
          ipb_pass_hash: "2",
          igneous: "",
          sk: "",
        },
      },
    )
  })

  it("importSession fails closed on application error", async () => {
    const http = mockHttp()
    http.post.mockResolvedValue({ error: 1, message: "invalid cookie format" })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(
      api.importSession({ sourceId: "ehentai.official", ipb_member_id: "bad", ipb_pass_hash: "bad" }),
    ).rejects.toThrow("invalid cookie format")
  })

  it("importSession rejects a semantic session validation failure", async () => {
    const http = mockHttp()
    http.post.mockResolvedValue({
      error: 0,
      data: {
        success: false,
        message: "ExHentai access requires igneous",
        ehentai_access: true,
        exhentai_access: false,
      },
    })
    const api = createR18Api(http as unknown as HttpClient)

    await expect(api.importSession({
      sourceId: "exhentai.official",
      ipb_member_id: "1",
      ipb_pass_hash: "2",
    })).rejects.toThrow("ExHentai access requires igneous")
  })
})
