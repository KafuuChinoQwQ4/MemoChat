import { describe, expect, it } from "vitest"
import {
  defaultSortForSource,
  filterTagOptions,
  sourceFilterConfig,
} from "./r18SourceFilters"

describe("r18SourceFilters", () => {
  it("exposes JM official ranking and Traditional Chinese tags from /categories", () => {
    const config = sourceFilterConfig("jm.official")
    expect(defaultSortForSource("jm.official")).toBe("mr")
    expect(config.sorts.map((item) => item.id)).toEqual(
      expect.arrayContaining(["mr", "mv_t", "mv_w", "mv_m", "mv", "mp", "ld", "tf"]),
    )
    // 官方主分類（繁體）
    expect(config.tags.some((item) => item.id === "同人")).toBe(true)
    expect(config.tags.some((item) => item.id === "單本")).toBe(true)
    expect(config.tags.some((item) => item.id === "韓漫")).toBe(true)
    expect(config.tags.some((item) => item.id === "漢化")).toBe(true)
    // 官方主題 block
    expect(config.tags.some((item) => item.id === "全彩")).toBe(true)
    expect(config.tags.some((item) => item.id === "劇情向")).toBe(true)
    expect(config.tags.some((item) => item.id === "NTR")).toBe(true)
    expect(config.tags.some((item) => item.id === "阿黑顏")).toBe(true)
    // must NOT use simplified-only chips that miss upstream index
    expect(config.tags.some((item) => item.id === "单行本")).toBe(false)
    expect(config.tags.some((item) => item.id === "韩漫")).toBe(false)
    expect(config.tags.length).toBeGreaterThan(40)
  })

  it("maps picacg / nhentai / ehentai / exhentai source-native options", () => {
    const picacg = sourceFilterConfig("picacg.official")
    expect(picacg.sorts.map((item) => item.id)).toEqual(
      expect.arrayContaining(["dd", "ld", "vd"]),
    )
    expect(picacg.tags.some((item) => item.id === "全彩")).toBe(true)
    expect(picacg.tags.some((item) => item.id === "長篇")).toBe(true)
    expect(picacg.tags.some((item) => item.id === "單行本")).toBe(true)
    // simplified chips removed
    expect(picacg.tags.some((item) => item.id === "长篇")).toBe(false)

    expect(sourceFilterConfig("nhentai.official").sorts.map((item) => item.id)).toEqual(
      expect.arrayContaining(["", "popular-today", "popular-week", "popular"]),
    )
    expect(sourceFilterConfig("nhentai.official").tags.some((item) => item.id === "language:chinese")).toBe(true)
    expect(sourceFilterConfig("nhentai.official").tags.some((item) => item.id === "category:doujinshi")).toBe(true)

    expect(sourceFilterConfig("ehentai.official").sorts.some((item) => item.id === "manga")).toBe(true)
    expect(sourceFilterConfig("exhentai.official").sorts.some((item) => item.id === "manga")).toBe(true)
    expect(sourceFilterConfig("ehentai.official").tags.some((item) => item.id === "female:big breasts")).toBe(true)
    expect(sourceFilterConfig("exhentai.official").tags.some((item) => item.id === "language:chinese")).toBe(true)
  })

  it("filters tag chips by local query while keeping the all option", () => {
    const tags = sourceFilterConfig("jm.official").tags
    const filtered = filterTagOptions(tags, "全彩")
    expect(filtered.some((item) => item.id === "")).toBe(true)
    expect(filtered.some((item) => item.id === "全彩")).toBe(true)
    expect(filtered.every((item) => !item.id || item.label.includes("全彩") || item.id.includes("全彩"))).toBe(true)
  })
})
