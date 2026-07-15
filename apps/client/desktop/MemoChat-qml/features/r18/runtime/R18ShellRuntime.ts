// Source file — compiled to R18ShellRuntime.js by `npm run build` (esbuild).
// Do NOT include .pragma library here; it is injected by esbuild as a banner.

interface TagBucket {
  name: string
  count: number
  id?: string
}

interface FilterOption {
  id: string
  label: string
}

interface FilterConfig {
  sorts: FilterOption[]
  tags: FilterOption[]
  tagPlaceholder: string
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
    buckets.push({ name, count: counts[name], id: name })
  }
  buckets.sort((a, b) => {
    if (b.count !== a.count) {
      return b.count - a.count
    }
    return a.name.localeCompare(b.name)
  })
  return buckets
}

/** Official per-source sort/tag chips aligned with web r18SourceFilters. */
export function sourceFilterConfig(sourceId: string | null | undefined): FilterConfig {
  const sid = String(sourceId || "")
  const empty: FilterConfig = {
    sorts: [{ id: "", label: "默认" }],
    tags: [{ id: "", label: "全部" }],
    tagPlaceholder: "tag / 分类",
  }
  if (sid === "jm.official") {
    return {
      sorts: [
        { id: "mr", label: "最新" },
        { id: "mv_t", label: "日排行" },
        { id: "mv_w", label: "週排行" },
        { id: "mv_m", label: "月排行" },
        { id: "mv", label: "總排行" },
        { id: "mp", label: "最多點贊" },
        { id: "ld", label: "最多愛心" },
        { id: "tf", label: "最多圖片" },
      ],
      tags: [
        { id: "", label: "全部" },
        { id: "同人", label: "同人" },
        { id: "單本", label: "單本" },
        { id: "短篇", label: "短篇" },
        { id: "其他類", label: "其他類" },
        { id: "韓漫", label: "韓漫" },
        { id: "English Manga", label: "English Manga" },
        { id: "一般向韓漫", label: "一般向韓漫" },
        { id: "Cosplay", label: "Cosplay" },
        { id: "3D", label: "3D" },
        { id: "禁漫漢化組", label: "禁漫漢化組" },
        { id: "漢化", label: "漢化" },
        { id: "日語", label: "日語" },
        { id: "CG圖集", label: "CG圖集" },
        { id: "禁漫去碼", label: "禁漫去碼" },
        { id: "禁漫上色", label: "禁漫上色" },
        { id: "青年漫", label: "青年漫" },
        { id: "劇情向", label: "劇情向" },
        { id: "校園", label: "校園" },
        { id: "純愛", label: "純愛" },
        { id: "人妻", label: "人妻" },
        { id: "師生", label: "師生" },
        { id: "近親", label: "近親" },
        { id: "百合", label: "百合" },
        { id: "YAOI", label: "YAOI" },
        { id: "性轉", label: "性轉" },
        { id: "NTR", label: "NTR" },
        { id: "偽娘", label: "偽娘" },
        { id: "癡女", label: "癡女" },
        { id: "全彩", label: "全彩" },
        { id: "女性向", label: "女性向" },
        { id: "蘿莉", label: "蘿莉" },
        { id: "禦姐", label: "禦姐" },
        { id: "熟女", label: "熟女" },
        { id: "正太", label: "正太" },
        { id: "巨乳", label: "巨乳" },
        { id: "貧乳", label: "貧乳" },
        { id: "女王", label: "女王" },
        { id: "教師", label: "教師" },
        { id: "女僕", label: "女僕" },
        { id: "護士", label: "護士" },
        { id: "泳裝", label: "泳裝" },
        { id: "眼鏡", label: "眼鏡" },
        { id: "連褲襪", label: "連褲襪" },
        { id: "兔女郎", label: "兔女郎" },
        { id: "群交", label: "群交" },
        { id: "足交", label: "足交" },
        { id: "SM", label: "SM" },
        { id: "肛交", label: "肛交" },
        { id: "阿黑顏", label: "阿黑顏" },
        { id: "藥物", label: "藥物" },
        { id: "扶他", label: "扶他" },
        { id: "調教", label: "調教" },
        { id: "野外露出", label: "野外露出" },
        { id: "催眠", label: "催眠" },
        { id: "自慰", label: "自慰" },
        { id: "觸手", label: "觸手" },
        { id: "獸交", label: "獸交" },
        { id: "CG集", label: "CG集" },
        { id: "重口", label: "重口" },
        { id: "獵奇", label: "獵奇" },
        { id: "非H", label: "非H" },
        { id: "血腥暴力", label: "血腥暴力" },
      ],
      tagPlaceholder: "官方繁體 tag，例如 同人、漢化",
    }
  }
  if (sid === "picacg.official") {
    return {
      sorts: [
        { id: "dd", label: "新到舊" },
        { id: "da", label: "舊到新" },
        { id: "ld", label: "最多愛心" },
        { id: "vd", label: "最多指名" },
      ],
      tags: [
        { id: "", label: "全部" },
        { id: "大家都在看", label: "大家都在看" },
        { id: "大濕推薦", label: "大濕推薦" },
        { id: "全彩", label: "全彩" },
        { id: "長篇", label: "長篇" },
        { id: "短篇", label: "短篇" },
        { id: "完結", label: "完結" },
        { id: "同人", label: "同人" },
        { id: "單行本", label: "單行本" },
        { id: "韓漫", label: "韓漫" },
        { id: "美漫", label: "美漫" },
        { id: "生肉", label: "生肉" },
        { id: "漢化", label: "漢化" },
        { id: "純愛", label: "純愛" },
        { id: "後宮", label: "後宮" },
        { id: "百合", label: "百合" },
        { id: "耽美", label: "耽美" },
        { id: "偽娘", label: "偽娘" },
        { id: "扶他", label: "扶他" },
        { id: "性轉換", label: "性轉換" },
        { id: "NTR", label: "NTR" },
        { id: "重口", label: "重口" },
        { id: "獵奇", label: "獵奇" },
        { id: "溫馨", label: "溫馨" },
        { id: "劇情", label: "劇情" },
        { id: "校園", label: "校園" },
        { id: "巨乳", label: "巨乳" },
        { id: "貧乳", label: "貧乳" },
        { id: "人妻", label: "人妻" },
        { id: "調教", label: "調教" },
        { id: "觸手", label: "觸手" },
        { id: "阿黑顏", label: "阿黑顏" },
      ],
      tagPlaceholder: "官方繁體分類，例如 全彩、長篇",
    }
  }
  if (sid === "nhentai.official") {
    return {
      sorts: [
        { id: "", label: "Recent" },
        { id: "popular-today", label: "Popular Today" },
        { id: "popular-week", label: "Popular Week" },
        { id: "popular-month", label: "Popular Month" },
        { id: "popular", label: "Popular All-time" },
      ],
      tags: [
        { id: "", label: "All" },
        { id: "language:chinese", label: "Chinese" },
        { id: "language:japanese", label: "Japanese" },
        { id: "language:english", label: "English" },
        { id: "language:translated", label: "Translated" },
        { id: "category:doujinshi", label: "Doujinshi" },
        { id: "category:manga", label: "Manga" },
        { id: "full color", label: "full color" },
        { id: "big breasts", label: "big breasts" },
        { id: "sole female", label: "sole female" },
        { id: "group", label: "group" },
        { id: "yuri", label: "yuri" },
        { id: "yaoi", label: "yaoi" },
        { id: "futanari", label: "futanari" },
        { id: "netorare", label: "netorare" },
        { id: "ahegao", label: "ahegao" },
        { id: "uncensored", label: "uncensored" },
      ],
      tagPlaceholder: "language:chinese / sole female",
    }
  }
  if (sid === "ehentai.official" || sid === "exhentai.official") {
    return {
      sorts: [
        { id: "", label: "All Categories" },
        { id: "doujinshi", label: "Doujinshi" },
        { id: "manga", label: "Manga" },
        { id: "artist_cg", label: "Artist CG" },
        { id: "game_cg", label: "Game CG" },
        { id: "western", label: "Western" },
        { id: "non-h", label: "Non-H" },
        { id: "image_set", label: "Image Set" },
        { id: "cosplay", label: "Cosplay" },
        { id: "asian_porn", label: "Asian Porn" },
        { id: "misc", label: "Misc" },
      ],
      tags: [
        { id: "", label: "No extra tag" },
        { id: "language:chinese", label: "language:chinese" },
        { id: "language:japanese", label: "language:japanese" },
        { id: "language:english", label: "language:english" },
        { id: "other:full color", label: "other:full color" },
        { id: "other:uncensored", label: "other:uncensored" },
        { id: "female:big breasts", label: "female:big breasts" },
        { id: "female:sole female", label: "female:sole female" },
        { id: "female:ahegao", label: "female:ahegao" },
        { id: "female:netorare", label: "female:netorare" },
        { id: "female:yuri", label: "female:yuri" },
        { id: "male:yaoi", label: "male:yaoi" },
      ],
      tagPlaceholder: "language:chinese / female:big breasts",
    }
  }
  return empty
}

export function defaultSortForSource(sourceId: string | null | undefined): string {
  const cfg = sourceFilterConfig(sourceId)
  return cfg.sorts[0]?.id || ""
}

export function mergeOfficialAndDerivedTags(
  sourceId: string | null | undefined,
  derivedBuckets: TagBucket[],
): Array<{ id: string; name: string; count: number; official: boolean }> {
  const cfg = sourceFilterConfig(sourceId)
  const official = (cfg.tags || [])
    .filter((t) => t && t.id)
    .map((t) => ({ id: t.id, name: t.label || t.id, count: 0, official: true }))
  const seen: Record<string, boolean> = {}
  for (const o of official) {
    seen[o.id] = true
  }
  for (const d of derivedBuckets || []) {
    const key = d.name || d.id || ""
    if (!key || seen[key]) continue
    official.push({ id: key, name: key, count: d.count || 0, official: false })
    seen[key] = true
  }
  return official
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
