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
})
