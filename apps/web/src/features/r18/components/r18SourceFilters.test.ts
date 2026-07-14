import { describe, expect, it } from "vitest"
import {
  defaultSortForSource,
  filterTagOptions,
  sourceFilterConfig,
} from "./r18SourceFilters"

describe("r18SourceFilters", () => {
  it("exposes JM hot ranking and expanded official tags", () => {
    const config = sourceFilterConfig("jm.official")
    expect(defaultSortForSource("jm.official")).toBe("mr")
    expect(config.sorts.map((item) => item.id)).toEqual(
      expect.arrayContaining(["mr", "mv_t", "mv_w", "mv_m", "mv", "mp"]),
    )
    expect(config.tags.some((item) => item.id === "同人")).toBe(true)
    expect(config.tags.some((item) => item.id === "全彩")).toBe(true)
    expect(config.tags.some((item) => item.id === "NTR")).toBe(true)
    // expanded list should be meaningfully larger than the old short chips
    expect(config.tags.length).toBeGreaterThan(20)
  })

  it("maps picacg / nhentai / ehentai source-native options", () => {
    expect(sourceFilterConfig("picacg.official").sorts.map((item) => item.id)).toEqual(
      expect.arrayContaining(["dd", "ld", "vd"]),
    )
    expect(sourceFilterConfig("nhentai.official").sorts.map((item) => item.id)).toEqual(
      expect.arrayContaining(["", "popular-today", "popular-week", "popular"]),
    )
    expect(sourceFilterConfig("ehentai.official").sorts.some((item) => item.id === "manga")).toBe(true)
    expect(sourceFilterConfig("nhentai.official").tags.some((item) => item.id === "language:chinese")).toBe(true)
    expect(sourceFilterConfig("ehentai.official").tags.some((item) => item.id === "female:big breasts")).toBe(true)
  })

  it("filters tag chips by local query while keeping the all option", () => {
    const tags = sourceFilterConfig("jm.official").tags
    const filtered = filterTagOptions(tags, "全彩")
    expect(filtered.some((item) => item.id === "")).toBe(true)
    expect(filtered.some((item) => item.id === "全彩")).toBe(true)
    expect(filtered.every((item) => !item.id || item.label.includes("全彩") || item.id.includes("全彩"))).toBe(true)
  })
})
