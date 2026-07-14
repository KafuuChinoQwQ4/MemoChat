/** R18 comic-list infinite scroll helpers. One UI load = one upstream source page for every source. */

/**
 * Fallback "full page" heuristic when a source omits max_page.
 * Common official adapters return ~40 comics per source page.
 */
export const R18_SEARCH_FULL_PAGE_FALLBACK_SIZE = 40

export const R18_SEARCH_SCROLL_LOAD_THRESHOLD_PX = 240

export interface R18SearchPageLike {
  items?: Array<{ comic_id?: string; source_id?: string }>
  max_page?: number
  error_message?: string
}

export function hasMoreR18SearchPages(
  page: number,
  maxPage: number | undefined,
  pageItemCount: number,
  fullPageFallbackSize: number = R18_SEARCH_FULL_PAGE_FALLBACK_SIZE,
): boolean {
  if (fullPageFallbackSize <= 0) return false
  if (typeof maxPage === "number" && Number.isFinite(maxPage) && maxPage > 0) {
    return page < maxPage
  }
  return pageItemCount >= fullPageFallbackSize
}

export function shouldLoadMoreR18SearchOnScroll(
  distanceToBottom: number,
  options: {
    hasMore: boolean
    isLoading: boolean
    isFetchingNextPage: boolean
    thresholdPx?: number
  },
): boolean {
  if (!options.hasMore || options.isLoading || options.isFetchingNextPage) {
    return false
  }
  return distanceToBottom <= (options.thresholdPx ?? R18_SEARCH_SCROLL_LOAD_THRESHOLD_PX)
}

export function mergeR18SearchItems<T extends { comic_id?: string; source_id?: string }>(
  existing: T[],
  incoming: T[],
): T[] {
  if (incoming.length === 0) return existing
  const seen = new Set(
    existing.map((item) => `${item.source_id ?? ""}:${item.comic_id ?? ""}`),
  )
  const merged = existing.slice()
  for (const item of incoming) {
    const key = `${item.source_id ?? ""}:${item.comic_id ?? ""}`
    if (seen.has(key)) continue
    seen.add(key)
    merged.push(item)
  }
  return merged
}

export function flattenR18SearchPages<T extends { comic_id?: string; source_id?: string }>(
  pages: Array<{ items?: T[] } | undefined>,
): T[] {
  return pages.reduce<T[]>((acc, page) => {
    if (!page?.items?.length) return acc
    return mergeR18SearchItems(acc, page.items)
  }, [])
}

export function latestR18SearchPage(pages: Array<R18SearchPageLike | undefined>): R18SearchPageLike | undefined {
  for (let index = pages.length - 1; index >= 0; index -= 1) {
    const page = pages[index]
    if (page) return page
  }
  return undefined
}
