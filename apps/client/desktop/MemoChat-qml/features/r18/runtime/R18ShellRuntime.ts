// Source file — compiled to R18ShellRuntime.js by `npm run build` (esbuild).
// Do NOT include .pragma library here; it is injected by esbuild as a banner.

interface TagBucket {
  name: string
  count: number
}

interface QmlListModel {
  count: number
  get(index: number): { tags?: unknown } | null
}

export function absoluteUrl(url: string | null | undefined, gatewayBaseUrl: string): string {
  if (!url) {
    return ""
  }
  if (
    url.indexOf("http://") === 0 ||
    url.indexOf("https://") === 0 ||
    url.indexOf("qrc:/") === 0
  ) {
    return url
  }
  if (url.indexOf("/") === 0 && gatewayBaseUrl.length > 0) {
    return gatewayBaseUrl + url
  }
  return url
}

export function normalizeTagArray(tags: unknown): string[] {
  if (!tags) {
    return []
  }
  if (typeof tags === "string") {
    return tags.length > 0 ? [tags] : []
  }
  if (!Array.isArray(tags)) {
    return []
  }
  const values: string[] = []
  for (let i = 0; i < tags.length; ++i) {
    const tag = String(tags[i] || "").trim()
    if (tag.length > 0) {
      values.push(tag)
    }
  }
  return values
}

export function buildSourceTagBuckets(model: QmlListModel | null | undefined): TagBucket[] {
  if (!model) {
    return []
  }
  const counts: Record<string, number> = {}
  const total = model.count !== undefined ? model.count : 0
  for (let i = 0; i < total; ++i) {
    const item = model.get(i)
    if (!item) {
      continue
    }
    const tags = normalizeTagArray(item.tags)
    for (let j = 0; j < tags.length; ++j) {
      const key = tags[j]
      if (!counts[key]) {
        counts[key] = 0
      }
      counts[key] += 1
    }
  }
  const buckets: TagBucket[] = []
  for (const name in counts) {
    buckets.push({ name, count: counts[name] })
  }
  buckets.sort((a, b) => {
    if (b.count !== a.count) {
      return b.count - a.count
    }
    return a.name.localeCompare(b.name)
  })
  return buckets
}

export function isGridAtBottom(
  gridView: { contentHeight: number; height: number; contentY: number } | null | undefined,
  threshold: number
): boolean {
  if (!gridView) {
    return false
  }
  return (
    gridView.contentHeight <= gridView.height ||
    gridView.contentY + gridView.height >= gridView.contentHeight - threshold
  )
}

export function modelCount(model: QmlListModel | null | undefined): number {
  return model && model.count !== undefined ? model.count : 0
}

export function currentComicId(currentComic: Record<string, string> | null | undefined): string {
  if (!currentComic) {
    return ""
  }
  return currentComic.comic_id || currentComic.id || ""
}

export function currentComicTitle(
  currentComic: Record<string, string> | null | undefined
): string {
  if (!currentComic) {
    return ""
  }
  return currentComic.title || currentComic.name || ""
}
