import { afterEach, beforeEach, describe, expect, it, vi } from "vitest"
import {
  clearMediaAuthCache,
  loadMediaAuthUrl,
  mediaAuthCacheSizeForTests,
  peekMediaAuthCache,
} from "./mediaAuthCache"

describe("mediaAuthCache", () => {
  const originalCreateObjectUrlDescriptor = Object.getOwnPropertyDescriptor(URL, "createObjectURL")
  const originalRevokeObjectUrlDescriptor = Object.getOwnPropertyDescriptor(URL, "revokeObjectURL")
  const createObjectUrlMock = vi.fn(() => "")
  const revokeObjectUrlMock = vi.fn()
  let created = 0

  beforeEach(() => {
    clearMediaAuthCache()
    created = 0
    createObjectUrlMock.mockReset()
    createObjectUrlMock.mockImplementation(() => {
      created += 1
      return `blob:test-${created}`
    })
    revokeObjectUrlMock.mockReset()
    Object.defineProperty(URL, "createObjectURL", { configurable: true, value: createObjectUrlMock })
    Object.defineProperty(URL, "revokeObjectURL", { configurable: true, value: revokeObjectUrlMock })
  })

  afterEach(() => {
    clearMediaAuthCache()
    if (originalCreateObjectUrlDescriptor) {
      Object.defineProperty(URL, "createObjectURL", originalCreateObjectUrlDescriptor)
    } else {
      Reflect.deleteProperty(URL, "createObjectURL")
    }
    if (originalRevokeObjectUrlDescriptor) {
      Object.defineProperty(URL, "revokeObjectURL", originalRevokeObjectUrlDescriptor)
    } else {
      Reflect.deleteProperty(URL, "revokeObjectURL")
    }
    vi.restoreAllMocks()
  })

  it("shares one network fetch across concurrent callers for the same key", async () => {
    const fetchMock = vi.fn(() =>
      Promise.resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 })),
    )
    vi.stubGlobal("fetch", fetchMock)

    const [a, b] = await Promise.all([
      loadMediaAuthUrl("/media/download?asset=a", "tok"),
      loadMediaAuthUrl("/media/download?asset=a", "tok"),
    ])

    expect(fetchMock).toHaveBeenCalledTimes(1)
    expect(a).toBe(b)
    expect(a).toBe("blob:test-1")
    expect(peekMediaAuthCache("/media/download?asset=a", "tok")).toBe("blob:test-1")
  })

  it("keeps a shared fetch alive while another same-key subscriber remains", async () => {
    let release: (() => void) | undefined
    let fetchSignal: AbortSignal | undefined
    const fetchMock = vi.fn((_url: string | URL | Request, init?: RequestInit) => new Promise<Response>((resolve, reject) => {
      fetchSignal = init?.signal ?? undefined
      fetchSignal?.addEventListener("abort", () => reject(new DOMException("aborted", "AbortError")), { once: true })
      release = () => resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 }))
    }))
    vi.stubGlobal("fetch", fetchMock)

    const firstController = new AbortController()
    const secondController = new AbortController()
    const first = loadMediaAuthUrl("/media/download?asset=shared", "tok", firstController.signal)
    const second = loadMediaAuthUrl("/media/download?asset=shared", "tok", secondController.signal)

    firstController.abort()

    await expect(first).rejects.toMatchObject({ name: "AbortError" })
    expect(fetchSignal?.aborted).toBe(false)
    release?.()
    await expect(second).resolves.toBe("blob:test-1")
    expect(fetchMock).toHaveBeenCalledTimes(1)
  })

  it("lets a later same-key subscriber cancel without aborting an earlier subscriber", async () => {
    let release: (() => void) | undefined
    let fetchSignal: AbortSignal | undefined
    const fetchMock = vi.fn((_url: string | URL | Request, init?: RequestInit) => new Promise<Response>((resolve, reject) => {
      fetchSignal = init?.signal ?? undefined
      fetchSignal?.addEventListener("abort", () => reject(new DOMException("aborted", "AbortError")), { once: true })
      release = () => resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 }))
    }))
    vi.stubGlobal("fetch", fetchMock)

    const secondController = new AbortController()
    const first = loadMediaAuthUrl("/media/download?asset=late-abort", "tok")
    const second = loadMediaAuthUrl("/media/download?asset=late-abort", "tok", secondController.signal)

    secondController.abort()

    await expect(second).rejects.toMatchObject({ name: "AbortError" })
    expect(fetchSignal?.aborted).toBe(false)
    release?.()
    await expect(first).resolves.toBe("blob:test-1")
    expect(fetchMock).toHaveBeenCalledTimes(1)
  })

  it("cancels a shared fetch after its final subscriber leaves and allows a fresh remount", async () => {
    const fetchMock = vi.fn((_url: string | URL | Request, init?: RequestInit) => new Promise<Response>((resolve, reject) => {
      const signal = init?.signal
      signal?.addEventListener("abort", () => reject(new DOMException("aborted", "AbortError")), { once: true })
      if (fetchMock.mock.calls.length === 2) {
        resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 }))
      }
    }))
    vi.stubGlobal("fetch", fetchMock)

    const firstController = new AbortController()
    const secondController = new AbortController()
    const first = loadMediaAuthUrl("/media/download?asset=remount", "tok", firstController.signal)
    const second = loadMediaAuthUrl("/media/download?asset=remount", "tok", secondController.signal)

    firstController.abort()
    await expect(first).rejects.toMatchObject({ name: "AbortError" })
    secondController.abort()
    await expect(second).rejects.toMatchObject({ name: "AbortError" })

    await expect(loadMediaAuthUrl("/media/download?asset=remount", "tok")).resolves.toBe("blob:test-1")
    expect(fetchMock).toHaveBeenCalledTimes(2)
  })

  it("reuses a ready entry without refetching (dialog remount)", async () => {
    const fetchMock = vi.fn(() =>
      Promise.resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 })),
    )
    vi.stubGlobal("fetch", fetchMock)

    const first = await loadMediaAuthUrl("/media/download?asset=b", "tok")
    const second = await loadMediaAuthUrl("/media/download?asset=b", "tok")

    expect(fetchMock).toHaveBeenCalledTimes(1)
    expect(first).toBe(second)
    expect(mediaAuthCacheSizeForTests()).toBe(1)
  })

  it("isolates cache entries by token (account switch safety)", async () => {
    const fetchMock = vi.fn(() =>
      Promise.resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 })),
    )
    vi.stubGlobal("fetch", fetchMock)

    await loadMediaAuthUrl("/media/download?asset=c", "tok-a")
    await loadMediaAuthUrl("/media/download?asset=c", "tok-b")

    expect(fetchMock).toHaveBeenCalledTimes(2)
    expect(peekMediaAuthCache("/media/download?asset=c", "tok-a")).toBe("blob:test-1")
    expect(peekMediaAuthCache("/media/download?asset=c", "tok-b")).toBe("blob:test-2")
  })

  it("clears and revokes object URLs on logout", async () => {
    const fetchMock = vi.fn(() =>
      Promise.resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 })),
    )
    vi.stubGlobal("fetch", fetchMock)

    await loadMediaAuthUrl("/media/download?asset=d", "tok")
    clearMediaAuthCache()

    expect(mediaAuthCacheSizeForTests()).toBe(0)
    expect(revokeObjectUrlMock).toHaveBeenCalledWith("blob:test-1")
    expect(peekMediaAuthCache("/media/download?asset=d", "tok")).toBeNull()
  })

  it("bounds concurrent fetches for different media URLs", async () => {
    const releases: Array<() => void> = []
    let active = 0
    let peakActive = 0
    const fetchMock = vi.fn(() => new Promise<Response>((resolve) => {
      active += 1
      peakActive = Math.max(peakActive, active)
      releases.push(() => {
        active -= 1
        resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 }))
      })
    }))
    vi.stubGlobal("fetch", fetchMock)

    const requests = Array.from({ length: 7 }, (_, index) => (
      loadMediaAuthUrl(`/api/r18/image?image_id=${index}`, "tok")
    ))
    await Promise.resolve()

    expect(fetchMock).toHaveBeenCalledTimes(4)
    expect(peakActive).toBeLessThanOrEqual(4)

    while (releases.length > 0) {
      releases.shift()?.()
      await Promise.resolve()
      await Promise.resolve()
    }
    await expect(Promise.all(requests)).resolves.toHaveLength(7)
    expect(fetchMock).toHaveBeenCalledTimes(7)
    expect(peakActive).toBeLessThanOrEqual(4)
  })

  it("removes aborted media requests from the pending queue", async () => {
    const releases: Array<() => void> = []
    const fetchMock = vi.fn(() => new Promise<Response>((resolve) => {
      releases.push(() => {
        resolve(new Response(new Blob(["img"], { type: "image/png" }), { status: 200 }))
      })
    }))
    vi.stubGlobal("fetch", fetchMock)

    const controllers = Array.from({ length: 7 }, () => new AbortController())
    const requests = controllers.map((controller, index) => (
      loadMediaAuthUrl(`/api/r18/image?image_id=abort-${index}`, "tok", controller.signal)
    ))
    await Promise.resolve()

    expect(fetchMock).toHaveBeenCalledTimes(4)
    controllers.slice(4).forEach((controller) => controller.abort())
    await Promise.allSettled(requests.slice(4))

    while (releases.length > 0) {
      releases.shift()?.()
      await Promise.resolve()
      await Promise.resolve()
    }
    await expect(Promise.all(requests.slice(0, 4))).resolves.toHaveLength(4)
    expect(fetchMock).toHaveBeenCalledTimes(4)
  })
})
