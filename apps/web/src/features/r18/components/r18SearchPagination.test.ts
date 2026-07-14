import { describe, expect, it } from "vitest"
import {
  flattenR18SearchPages,
  hasMoreR18SearchPages,
  mergeR18SearchItems,
  R18_SEARCH_FULL_PAGE_FALLBACK_SIZE,
  shouldLoadMoreR18SearchOnScroll,
} from "./r18SearchPagination"

describe("r18SearchPagination", () => {
  it("uses one upstream-page full-size fallback for every source", () => {
    expect(R18_SEARCH_FULL_PAGE_FALLBACK_SIZE).toBe(40)
  })

  it("prefers max_page when the source reports it", () => {
    expect(hasMoreR18SearchPages(1, 3, 12)).toBe(true)
    expect(hasMoreR18SearchPages(3, 3, 40)).toBe(false)
  })

  it("falls back to full-page heuristics when max_page is missing", () => {
    expect(hasMoreR18SearchPages(1, undefined, 40)).toBe(true)
    expect(hasMoreR18SearchPages(2, undefined, 12)).toBe(false)
    expect(hasMoreR18SearchPages(1, 0, 40)).toBe(true)
  })

  it("only auto-loads near the bottom while idle with more pages", () => {
    expect(
      shouldLoadMoreR18SearchOnScroll(120, {
        hasMore: true,
        isLoading: false,
        isFetchingNextPage: false,
      }),
    ).toBe(true)
    expect(
      shouldLoadMoreR18SearchOnScroll(400, {
        hasMore: true,
        isLoading: false,
        isFetchingNextPage: false,
      }),
    ).toBe(false)
    expect(
      shouldLoadMoreR18SearchOnScroll(80, {
        hasMore: true,
        isLoading: false,
        isFetchingNextPage: true,
      }),
    ).toBe(false)
    expect(
      shouldLoadMoreR18SearchOnScroll(80, {
        hasMore: false,
        isLoading: false,
        isFetchingNextPage: false,
      }),
    ).toBe(false)
  })

  it("dedupes comic rows while flattening infinite-query pages", () => {
    const pages = [
      {
        items: [
          { source_id: "jm.official", comic_id: "1", title: "A" },
          { source_id: "jm.official", comic_id: "2", title: "B" },
        ],
      },
      {
        items: [
          { source_id: "jm.official", comic_id: "2", title: "B-dup" },
          { source_id: "jm.official", comic_id: "3", title: "C" },
        ],
      },
    ]
    expect(flattenR18SearchPages(pages)).toEqual([
      { source_id: "jm.official", comic_id: "1", title: "A" },
      { source_id: "jm.official", comic_id: "2", title: "B" },
      { source_id: "jm.official", comic_id: "3", title: "C" },
    ])
    expect(
      mergeR18SearchItems(
        [{ source_id: "jm.official", comic_id: "1" }],
        [{ source_id: "jm.official", comic_id: "1" }, { source_id: "jm.official", comic_id: "4" }],
      ),
    ).toEqual([
      { source_id: "jm.official", comic_id: "1" },
      { source_id: "jm.official", comic_id: "4" },
    ])
  })
})
