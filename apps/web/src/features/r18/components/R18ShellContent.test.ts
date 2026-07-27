import { QueryClient, QueryClientProvider } from "@tanstack/react-query"
import { act, fireEvent, render, screen, waitFor } from "@testing-library/react"
import { createElement } from "react"
import { readFileSync } from "node:fs"
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest"
import { useSessionStore } from "@/core/session/sessionStore"
import type { R18VideoPlayback } from "@/features/r18/api/r18Api"
import { clearMediaAuthCache } from "@/shared/media/mediaAuthCache"
import { ReaderPage, R18ShellContent } from "./R18ShellContent"
import { R18VideoPlayerOverlay } from "./R18VideoPlayerOverlay"
import {
  accountInteractionKind,
  isActionableSource,
  type AccountInteractionKind,
} from "./r18SourceAvailability"

const gatewayHttp = vi.hoisted(() => ({
  get: vi.fn(),
  post: vi.fn(),
}))

vi.mock("@/shared/gateway/ClientGateway", () => ({
  getGateway: () => ({ http: gatewayHttp }),
}))

function loginForR18Test() {
  useSessionStore.getState().clearSession()
  useSessionStore.getState().setLogin({
    uid: 42,
    token: "tok",
    loginTicket: "ticket",
    ticketExpireMs: Date.now() + 60_000,
    protocolVersion: 3,
    chatEndpoints: [],
    profile: { uid: 42, name: "tester", email: "t@example.test", icon: "" },
  })
}

function playback(qualities: number[] = [1080, 720, 480]): R18VideoPlayback {
  return {
    source_id: "hanime1.official",
    chapter_id: "407339",
    poster: "https://poster.example/407339.jpg",
    expires_at_ms: 1_785_121_222_000,
    sources: qualities.map((quality) => ({
      url: `https://vdownload.hembed.com/407339-${quality}p.mp4?secure=test,1785121222`,
      mime_type: "video/mp4" as const,
      quality,
    })),
  }
}

function renderPlayer(
  resolveVideo: (
    sourceId: string,
    chapterId: string,
    signal: AbortSignal,
  ) => Promise<R18VideoPlayback>,
  onClose = vi.fn(),
) {
  return {
    onClose,
    ...render(createElement(R18VideoPlayerOverlay, {
      sourceId: "hanime1.official",
      chapterId: "407339",
      title: "站内播放测试",
      resolveVideo,
      onClose,
    })),
  }
}

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
    ["exhentai.official", "required-ehentai-auth"],
    ["nhentai.official", "optional-account-or-cookie"],
  ])("maps %s to an actionable account mode", (sourceId, expected) => {
    expect(accountInteractionKind({
      source_id: sourceId,
      auth_required: sourceId === "picacg.official" || sourceId === "exhentai.official",
    })).toBe(expected)
  })
})

describe("R18 reader page", () => {
  beforeEach(() => {
    clearMediaAuthCache()
    loginForR18Test()
  })

  afterEach(() => {
    clearMediaAuthCache()
    act(() => useSessionStore.getState().clearSession())
    vi.unstubAllGlobals()
  })

  it("replaces the spinner with a terminal error when an image fetch fails", async () => {
    let resolveFetch: ((response: Response) => void) | undefined
    vi.stubGlobal("fetch", vi.fn(() => new Promise<Response>((resolve) => {
      resolveFetch = resolve
    })))

    render(createElement(ReaderPage, { url: "/api/r18/image?source_id=jm.official&image_url=bad" }))
    await act(async () => {
      resolveFetch?.(new Response("failed", { status: 502 }))
      await Promise.resolve()
    })

    await waitFor(() => expect(screen.getByText("图片加载失败")).toBeInTheDocument())
    expect(screen.queryByRole("status")).not.toBeInTheDocument()
    expect(screen.getByRole("button", { name: "重试" })).toBeInTheDocument()
  })
})

describe("R18 Hanime1 video player", () => {
  afterEach(() => {
    gatewayHttp.get.mockReset()
    gatewayHttp.post.mockReset()
    act(() => useSessionStore.getState().clearSession())
    vi.restoreAllMocks()
  })

  it("shows loading while resolving and aborts the request on close", () => {
    let observedSignal: AbortSignal | undefined
    const resolveVideo = vi.fn((_sourceId: string, _chapterId: string, signal: AbortSignal) => {
      observedSignal = signal
      return new Promise<R18VideoPlayback>(() => undefined)
    })
    const { onClose, unmount } = renderPlayer(resolveVideo)

    expect(screen.getByRole("status")).toHaveTextContent("正在解析视频地址")
    fireEvent.click(screen.getByRole("button", { name: "关闭播放器" }))
    expect(onClose).toHaveBeenCalledTimes(1)
    unmount()
    expect(observedSignal?.aborted).toBe(true)
  })

  it("defaults to 720p and renders native inline metadata controls", async () => {
    const resolveVideo = vi.fn().mockResolvedValue(playback())
    renderPlayer(resolveVideo)

    const video = await screen.findByLabelText("站内播放测试 视频播放器")
    expect(resolveVideo).toHaveBeenCalledWith("hanime1.official", "407339", expect.any(AbortSignal))
    expect(video).toHaveAttribute("src", expect.stringContaining("407339-720p.mp4"))
    expect(video).toHaveAttribute("controls")
    expect(video).toHaveAttribute("playsinline")
    expect(video).toHaveAttribute("preload", "metadata")
    await waitFor(() => {
      expect(video).toHaveAttribute("poster", "https://poster.example/407339.jpg")
    })
    expect(screen.getByRole("combobox", { name: "画质" })).toHaveValue("720")
    expect(screen.queryByRole("link", { name: "在官网打开" })).not.toBeInTheDocument()
  })

  it("falls back to the highest quality when 720p is unavailable", async () => {
    renderPlayer(vi.fn().mockResolvedValue(playback([1080, 480])))

    const video = await screen.findByLabelText("站内播放测试 视频播放器")
    expect(video).toHaveAttribute("src", expect.stringContaining("407339-1080p.mp4"))
    expect(screen.getByRole("combobox", { name: "画质" })).toHaveValue("1080")
  })

  it("preserves time and resumes active playback after a quality change", async () => {
    renderPlayer(vi.fn().mockResolvedValue(playback()))
    const video = await screen.findByLabelText<HTMLVideoElement>("站内播放测试 视频播放器")
    const play = vi.fn().mockResolvedValue(undefined)
    Object.defineProperty(video, "paused", { configurable: true, value: false })
    Object.defineProperty(video, "play", { configurable: true, value: play })
    video.currentTime = 37

    fireEvent.change(screen.getByRole("combobox", { name: "画质" }), { target: { value: "1080" } })
    expect(video).toHaveAttribute("src", expect.stringContaining("407339-1080p.mp4"))
    fireEvent.loadedMetadata(video)

    expect(video.currentTime).toBe(37)
    expect(play).toHaveBeenCalledTimes(1)
  })

  it("shows resolve errors with retry and an error-only official fallback", async () => {
    const resolveVideo = vi.fn()
      .mockRejectedValueOnce(new Error("上游暂时不可用"))
      .mockResolvedValueOnce(playback())
    renderPlayer(resolveVideo)

    expect(await screen.findByRole("alert")).toHaveTextContent("上游暂时不可用")
    expect(screen.getByRole("link", { name: "在官网打开" })).toHaveAttribute(
      "href",
      "https://hanime1.me/watch?v=407339",
    )
    fireEvent.click(screen.getByRole("button", { name: "重试" }))

    expect(await screen.findByLabelText(/站内播放测试 .*视频播放器/)).toBeInTheDocument()
    expect(resolveVideo).toHaveBeenCalledTimes(2)
    expect(screen.queryByRole("link", { name: "在官网打开" })).not.toBeInTheDocument()
  })

  it("keeps quality controls available after a media error and retries resolution", async () => {
    const resolveVideo = vi.fn().mockResolvedValue(playback())
    renderPlayer(resolveVideo)
    const video = await screen.findByLabelText("站内播放测试 视频播放器")

    fireEvent.error(video)

    expect(screen.getByRole("alert")).toHaveTextContent("视频播放失败")
    expect(screen.getByRole("combobox", { name: "画质" })).toBeInTheDocument()
    expect(screen.getByRole("link", { name: "在官网打开" })).toBeInTheDocument()
    fireEvent.click(screen.getByRole("button", { name: "重试" }))
    await waitFor(() => expect(resolveVideo).toHaveBeenCalledTimes(2))
  })

  it("closes on Escape", async () => {
    const { onClose } = renderPlayer(vi.fn().mockResolvedValue(playback()))
    await screen.findByLabelText("站内播放测试 视频播放器")

    fireEvent.keyDown(document, { key: "Escape" })
    expect(onClose).toHaveBeenCalledTimes(1)
  })

  it("keeps native video controls reachable after the quality selector", async () => {
    renderPlayer(vi.fn().mockResolvedValue(playback()))
    await screen.findByLabelText("站内播放测试 视频播放器")
    const quality = screen.getByRole("combobox", { name: "画质" })
    quality.focus()

    const tab = new KeyboardEvent("keydown", { key: "Tab", bubbles: true, cancelable: true })
    document.dispatchEvent(tab)

    expect(tab.defaultPrevented).toBe(false)
    expect(document.activeElement).toBe(quality)
  })

  it("keeps card then chapter navigation and opens the successful Hanime1 path in-app", async () => {
    loginForR18Test()
    gatewayHttp.get.mockImplementation((path: string) => {
      if (path === "/api/r18/access") {
        return Promise.resolve({
          error: 0,
          data: { allowed: true, adult_attested_at_ms: 1, state: "allowed", can_attest: true },
        })
      }
      if (path === "/api/r18/sources") {
        return Promise.resolve({
          error: 0,
          data: { sources: [{ id: "hanime1.official", title: "hanime1", enabled: true, status: "ok" }] },
        })
      }
      if (path === "/api/r18/accounts") return Promise.resolve({ error: 0, data: { managed: [] } })
      if (path === "/api/r18/library") return Promise.resolve({ error: 0, data: { folders: [], items: [] } })
      return Promise.reject(new Error(`unexpected GET ${path}`))
    })
    gatewayHttp.post.mockImplementation((path: string) => {
      if (path === "/api/r18/search") {
        return Promise.resolve({
          error: 0,
          data: {
            source_id: "hanime1.official",
            max_page: 1,
            items: [{
              source_id: "hanime1.official",
              comic_id: "407339",
              title: "站内播放测试",
              cover: "https://poster.example/search.jpg",
            }],
          },
        })
      }
      if (path === "/api/r18/comic/detail") {
        return Promise.resolve({
          error: 0,
          data: { chapters: [{ chapter_id: "407339", title: "完整視頻", index: 0 }] },
        })
      }
      if (path === "/api/r18/video/resolve") return Promise.resolve({ error: 0, data: playback() })
      return Promise.reject(new Error(`unexpected POST ${path}`))
    })
    const windowOpen = vi.spyOn(window, "open").mockImplementation(() => null)
    const queryClient = new QueryClient({
      defaultOptions: { queries: { retry: false }, mutations: { retry: false } },
    })
    render(createElement(
      QueryClientProvider,
      { client: queryClient },
      createElement(R18ShellContent),
    ))

    const cardTitle = await screen.findByText("站内播放测试")
    const card = cardTitle.closest<HTMLElement>('[role="button"]')
    expect(card).not.toBeNull()
    fireEvent.click(card as HTMLElement)
    fireEvent.click(await screen.findByRole("button", { name: "完整視頻" }))

    expect(await screen.findByLabelText(/站内播放测试 .*视频播放器/)).toBeInTheDocument()
    expect(windowOpen).not.toHaveBeenCalled()
  })
})

describe("R18 playback document policy", () => {
  it("delivers no-referrer and allows only the Hanime1 CDN for media", () => {
    const indexHtml = readFileSync("index.html", "utf8")
    const viteTs = readFileSync("vite.config.ts", "utf8")
    const viteJs = readFileSync("vite.config.js", "utf8")

    expect(indexHtml).toContain('<meta name="referrer" content="no-referrer"')
    expect(indexHtml).toContain("media-src 'self' https://vdownload.hembed.com")
    for (const config of [viteTs, viteJs]) {
      expect(config).toContain('"media-src \'self\' https://vdownload.hembed.com"')
      expect(config).toContain('"Referrer-Policy": "no-referrer"')
    }
  })
})
