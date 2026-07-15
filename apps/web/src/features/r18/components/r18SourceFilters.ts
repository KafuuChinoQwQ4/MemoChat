/** Per-source sort / category / tag filter options for R18 browse UI.
 *
 * Tag / category ids must match what each upstream source actually indexes.
 * JM / Picacg are Traditional Chinese (繁體) — Simplified chips often return 0 hits.
 * Values below were aligned with official endpoints (JM `/categories`, Picacg category catalog,
 * nHentai language/tag query syntax, e-hentai f_cats + namespace:tag).
 */

export interface R18FilterOption {
  id: string
  label: string
}

export interface R18SourceFilterConfig {
  /** Native sort / ranking options exposed by the official source. */
  sorts: R18FilterOption[]
  /** Optional preset category / tag chips. Free-form tag still allowed via input. */
  tags: R18FilterOption[]
  /** Placeholder for the free-form tag field. */
  tagPlaceholder: string
  /** Whether free-form tag input is shown. */
  allowCustomTag: boolean
}

// ─── 禁漫天堂 JMComic ────────────────────────────────────────────────────────
// Sort codes from official app (`o=`): mr/mv/mv_t/mv_w/mv_m/mp/ld/tf
// Categories + theme blocks from GET /categories (decrypted official payload).

const JM_SORTS: R18FilterOption[] = [
  { id: "mr", label: "最新" },
  { id: "mv_t", label: "日排行" },
  { id: "mv_w", label: "週排行" },
  { id: "mv_m", label: "月排行" },
  { id: "mv", label: "總排行" },
  { id: "mp", label: "最多點贊" },
  { id: "ld", label: "最多愛心" },
  { id: "tf", label: "最多圖片" },
]

/** Official JM categories + sub-categories + theme blocks (繁體原文，作 search_query). */
const JM_TAGS: R18FilterOption[] = [
  { id: "", label: "全部" },
  // —— 主分類（/categories.categories）——
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
  // —— 子分類 ——
  { id: "漢化", label: "漢化" },
  { id: "日語", label: "日語" },
  { id: "CG圖集", label: "CG圖集" },
  { id: "禁漫去碼", label: "禁漫去碼" },
  { id: "禁漫上色", label: "禁漫上色" },
  { id: "青年漫", label: "青年漫" },
  { id: "其他漫畫", label: "其他漫畫" },
  { id: "角色扮演", label: "角色扮演" },
  { id: "IRODORI", label: "IRODORI" },
  { id: "FAKKU", label: "FAKKU" },
  { id: "18scan", label: "18scan" },
  { id: "Manhwa", label: "Manhwa" },
  { id: "Comic", label: "Comic" },
  // —— 主題A漫（blocks）——
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
  // —— 角色 / 扮演 ——
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
  { id: "其他制服", label: "其他制服" },
  { id: "兔女郎", label: "兔女郎" },
  // —— 特殊 PLAY ——
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
  // —— 其他 ——
  { id: "CG集", label: "CG集" },
  { id: "重口", label: "重口" },
  { id: "獵奇", label: "獵奇" },
  { id: "非H", label: "非H" },
  { id: "血腥暴力", label: "血腥暴力" },
]

// ─── 哔咔 Picacg ─────────────────────────────────────────────────────────────
// sort: dd/da/ld/vd (官方 comics 排序)
// categories: 官方分類名為繁體，搜索 keyword 即分類/標籤原文。

const PICACG_SORTS: R18FilterOption[] = [
  { id: "dd", label: "新到舊" },
  { id: "da", label: "舊到新" },
  { id: "ld", label: "最多愛心" },
  { id: "vd", label: "最多指名" },
]

/** Official Picacg category titles (繁體). */
const PICACG_TAGS: R18FilterOption[] = [
  { id: "", label: "全部" },
  // 官方首頁 / 導航常見分類
  { id: "大家都在看", label: "大家都在看" },
  { id: "大濕推薦", label: "大濕推薦" },
  { id: "那年今天", label: "那年今天" },
  { id: "官方都在看", label: "官方都在看" },
  // 內容形態
  { id: "全彩", label: "全彩" },
  { id: "長篇", label: "長篇" },
  { id: "短篇", label: "短篇" },
  { id: "完結", label: "完結" },
  { id: "未完結", label: "未完結" },
  { id: "同人", label: "同人" },
  { id: "單行本", label: "單行本" },
  { id: "雜誌", label: "雜誌" },
  { id: "韓漫", label: "韓漫" },
  { id: "美漫", label: "美漫" },
  { id: "日漫", label: "日漫" },
  { id: "生肉", label: "生肉" },
  { id: "漢化", label: "漢化" },
  { id: "翻譯", label: "翻譯" },
  { id: "CG雜圖", label: "CG雜圖" },
  { id: "Cosplay", label: "Cosplay" },
  { id: "寫真", label: "寫真" },
  // 向別 / 題材
  { id: "純愛", label: "純愛" },
  { id: "戀愛", label: "戀愛" },
  { id: "後宮", label: "後宮" },
  { id: "百合", label: "百合" },
  { id: "耽美", label: "耽美" },
  { id: "偽娘", label: "偽娘" },
  { id: "扶他", label: "扶他" },
  { id: "性轉換", label: "性轉換" },
  { id: "性轉", label: "性轉" },
  { id: "NTR", label: "NTR" },
  { id: "重口", label: "重口" },
  { id: "獵奇", label: "獵奇" },
  { id: "溫馨", label: "溫馨" },
  { id: "劇情", label: "劇情" },
  { id: "校園", label: "校園" },
  { id: "幻想", label: "幻想" },
  { id: "科幻", label: "科幻" },
  { id: "戰鬥", label: "戰鬥" },
  { id: "懸疑", label: "懸疑" },
  { id: "歷史", label: "歷史" },
  { id: "其他", label: "其他" },
  { id: "紳士", label: "紳士" },
  { id: "色情", label: "色情" },
  // 常見人物 / play 標籤（官方庫內索引的繁體寫法）
  { id: "巨乳", label: "巨乳" },
  { id: "貧乳", label: "貧乳" },
  { id: "人妻", label: "人妻" },
  { id: "教師", label: "教師" },
  { id: "姊妹", label: "姊妹" },
  { id: "母女", label: "母女" },
  { id: "調教", label: "調教" },
  { id: "束縛", label: "束縛" },
  { id: "觸手", label: "觸手" },
  { id: "異種", label: "異種" },
  { id: "怪物", label: "怪物" },
  { id: "機械", label: "機械" },
  { id: "群P", label: "群P" },
  { id: "亂倫", label: "亂倫" },
  { id: "露出", label: "露出" },
  { id: "催眠", label: "催眠" },
  { id: "強制", label: "強制" },
  { id: "凌辱", label: "凌辱" },
  { id: "SM", label: "SM" },
  { id: "足控", label: "足控" },
  { id: "眼鏡", label: "眼鏡" },
  { id: "制服", label: "制服" },
  { id: "泳裝", label: "泳裝" },
  { id: "絲襪", label: "絲襪" },
  { id: "中出", label: "中出" },
  { id: "懷孕", label: "懷孕" },
  { id: "阿黑顏", label: "阿黑顏" },
  { id: "肛交", label: "肛交" },
  { id: "口交", label: "口交" },
]

// ─── nHentai ────────────────────────────────────────────────────────────────
// sort empty | popular | popular-today | popular-week | popular-month
// tags use official query syntax: language:… / tag:… / category:… / parody:… / character:… / artist:…

const NHENTAI_SORTS: R18FilterOption[] = [
  { id: "", label: "Recent" },
  { id: "popular-today", label: "Popular Today" },
  { id: "popular-week", label: "Popular Week" },
  { id: "popular-month", label: "Popular Month" },
  { id: "popular", label: "Popular All-time" },
]

/** Official nHentai languages + high-frequency tags / categories (English slug form). */
const NHENTAI_TAGS: R18FilterOption[] = [
  { id: "", label: "All" },
  // languages
  { id: "language:chinese", label: "Chinese" },
  { id: "language:japanese", label: "Japanese" },
  { id: "language:english", label: "English" },
  { id: "language:translated", label: "Translated" },
  { id: "language:rewrite", label: "Rewrite" },
  { id: "language:speechless", label: "Speechless" },
  { id: "language:text cleaned", label: "Text Cleaned" },
  // categories
  { id: "category:doujinshi", label: "Doujinshi" },
  { id: "category:manga", label: "Manga" },
  { id: "category:artistcg", label: "Artist CG" },
  { id: "category:gamecg", label: "Game CG" },
  { id: "category:western", label: "Western" },
  { id: "category:non-h", label: "Non-H" },
  { id: "category:imageset", label: "Image Set" },
  { id: "category:cosplay", label: "Cosplay" },
  { id: "category:asianporn", label: "Asian Porn" },
  { id: "category:misc", label: "Misc" },
  // high-frequency tags (official English names)
  { id: "full color", label: "full color" },
  { id: "big breasts", label: "big breasts" },
  { id: "small breasts", label: "small breasts" },
  { id: "sole female", label: "sole female" },
  { id: "sole male", label: "sole male" },
  { id: "group", label: "group" },
  { id: "lolicon", label: "lolicon" },
  { id: "shotacon", label: "shotacon" },
  { id: "milf", label: "milf" },
  { id: "schoolgirl uniform", label: "schoolgirl uniform" },
  { id: "stockings", label: "stockings" },
  { id: "glasses", label: "glasses" },
  { id: "nakadashi", label: "nakadashi" },
  { id: "ahegao", label: "ahegao" },
  { id: "blowjob", label: "blowjob" },
  { id: "anal", label: "anal" },
  { id: "ffm threesome", label: "ffm threesome" },
  { id: "mmf threesome", label: "mmf threesome" },
  { id: "yuri", label: "yuri" },
  { id: "yaoi", label: "yaoi" },
  { id: "futanari", label: "futanari" },
  { id: "crossdressing", label: "crossdressing" },
  { id: "gender bender", label: "gender bender" },
  { id: "netorare", label: "netorare" },
  { id: "cheating", label: "cheating" },
  { id: "incest", label: "incest" },
  { id: "mind control", label: "mind control" },
  { id: "rape", label: "rape" },
  { id: "bondage", label: "bondage" },
  { id: "bdsm", label: "bdsm" },
  { id: "tentacles", label: "tentacles" },
  { id: "monster", label: "monster" },
  { id: "elf", label: "elf" },
  { id: "demon girl", label: "demon girl" },
  { id: "dark skin", label: "dark skin" },
  { id: "blonde hair", label: "blonde hair" },
  { id: "twintails", label: "twintails" },
  { id: "ponytail", label: "ponytail" },
  { id: "mosaic censorship", label: "mosaic censorship" },
  { id: "uncensored", label: "uncensored" },
  { id: "multi-work series", label: "multi-work series" },
  { id: "anthology", label: "anthology" },
  { id: "swimsuit", label: "swimsuit" },
  { id: "pantyhose", label: "pantyhose" },
  { id: "maid", label: "maid" },
  { id: "nurse", label: "nurse" },
  { id: "teacher", label: "teacher" },
  { id: "impregnation", label: "impregnation" },
  { id: "lactation", label: "lactation" },
  { id: "footjob", label: "footjob" },
  { id: "paizuri", label: "paizuri" },
  { id: "femdom", label: "femdom" },
  { id: "ugly bastard", label: "ugly bastard" },
  { id: "mind break", label: "mind break" },
  { id: "drunk", label: "drunk" },
  { id: "sleeping", label: "sleeping" },
]

// ─── e-hentai / exhentai ─────────────────────────────────────────────────────
// "sort" chips map to f_cats category masks (adapter NormalizeEhentaiCats).
// tags use official namespace:name syntax.

const EHENTAI_SORTS: R18FilterOption[] = [
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
]

/** Official e-hentai namespace:tag chips (same index used by exhentai). */
const EHENTAI_TAGS: R18FilterOption[] = [
  { id: "", label: "No extra tag" },
  // language
  { id: "language:chinese", label: "language:chinese" },
  { id: "language:japanese", label: "language:japanese" },
  { id: "language:english", label: "language:english" },
  { id: "language:translated", label: "language:translated" },
  { id: "language:speechless", label: "language:speechless" },
  { id: "language:text cleaned", label: "language:text cleaned" },
  { id: "language:rewrite", label: "language:rewrite" },
  // other
  { id: "other:full color", label: "other:full color" },
  { id: "other:mosaic censorship", label: "other:mosaic censorship" },
  { id: "other:uncensored", label: "other:uncensored" },
  { id: "other:anthology", label: "other:anthology" },
  { id: "other:multi-work series", label: "other:multi-work series" },
  // female
  { id: "female:big breasts", label: "female:big breasts" },
  { id: "female:small breasts", label: "female:small breasts" },
  { id: "female:sole female", label: "female:sole female" },
  { id: "female:lolicon", label: "female:lolicon" },
  { id: "female:milf", label: "female:milf" },
  { id: "female:schoolgirl uniform", label: "female:schoolgirl uniform" },
  { id: "female:stockings", label: "female:stockings" },
  { id: "female:pantyhose", label: "female:pantyhose" },
  { id: "female:glasses", label: "female:glasses" },
  { id: "female:nakadashi", label: "female:nakadashi" },
  { id: "female:ahegao", label: "female:ahegao" },
  { id: "female:blowjob", label: "female:blowjob" },
  { id: "female:anal", label: "female:anal" },
  { id: "female:group", label: "female:group" },
  { id: "female:yuri", label: "female:yuri" },
  { id: "female:futanari", label: "female:futanari" },
  { id: "female:gender bender", label: "female:gender bender" },
  { id: "female:netorare", label: "female:netorare" },
  { id: "female:cheating", label: "female:cheating" },
  { id: "female:incest", label: "female:incest" },
  { id: "female:mind control", label: "female:mind control" },
  { id: "female:rape", label: "female:rape" },
  { id: "female:bondage", label: "female:bondage" },
  { id: "female:bdsm", label: "female:bdsm" },
  { id: "female:tentacles", label: "female:tentacles" },
  { id: "female:monster", label: "female:monster" },
  { id: "female:elf", label: "female:elf" },
  { id: "female:demon girl", label: "female:demon girl" },
  { id: "female:dark skin", label: "female:dark skin" },
  { id: "female:blonde hair", label: "female:blonde hair" },
  { id: "female:twintails", label: "female:twintails" },
  { id: "female:ponytail", label: "female:ponytail" },
  { id: "female:swimsuit", label: "female:swimsuit" },
  { id: "female:maid", label: "female:maid" },
  { id: "female:nurse", label: "female:nurse" },
  { id: "female:teacher", label: "female:teacher" },
  { id: "female:impregnation", label: "female:impregnation" },
  { id: "female:lactation", label: "female:lactation" },
  { id: "female:paizuri", label: "female:paizuri" },
  { id: "female:femdom", label: "female:femdom" },
  { id: "female:mind break", label: "female:mind break" },
  // male
  { id: "male:sole male", label: "male:sole male" },
  { id: "male:group", label: "male:group" },
  { id: "male:yaoi", label: "male:yaoi" },
  { id: "male:crossdressing", label: "male:crossdressing" },
  { id: "male:shotacon", label: "male:shotacon" },
  { id: "male:ugly bastard", label: "male:ugly bastard" },
  { id: "male:dilf", label: "male:dilf" },
]

const DEFAULT_FILTER: R18SourceFilterConfig = {
  sorts: [{ id: "", label: "默认排序" }],
  tags: [{ id: "", label: "全部" }],
  tagPlaceholder: "输入 tag / 分类后筛选",
  allowCustomTag: true,
}

const BY_SOURCE: Record<string, R18SourceFilterConfig> = {
  "jm.official": {
    sorts: JM_SORTS,
    tags: JM_TAGS,
    tagPlaceholder: "官方繁體 tag，例如 同人、漢化、劇情向、單本",
    allowCustomTag: true,
  },
  "picacg.official": {
    sorts: PICACG_SORTS,
    tags: PICACG_TAGS,
    tagPlaceholder: "官方繁體分類，例如 全彩、長篇、純愛",
    allowCustomTag: true,
  },
  "nhentai.official": {
    sorts: NHENTAI_SORTS,
    tags: NHENTAI_TAGS,
    tagPlaceholder: '官方查询，例如 language:chinese 或 sole female',
    allowCustomTag: true,
  },
  "ehentai.official": {
    sorts: EHENTAI_SORTS,
    tags: EHENTAI_TAGS,
    tagPlaceholder: "附加 namespace:tag，例如 language:chinese",
    allowCustomTag: true,
  },
  // exhentai 与 e-hentai 共用同一套分类 / tag 体系
  "exhentai.official": {
    sorts: EHENTAI_SORTS,
    tags: EHENTAI_TAGS,
    tagPlaceholder: "附加 namespace:tag，例如 language:chinese",
    allowCustomTag: true,
  },
}

export function sourceFilterConfig(sourceId?: string | null): R18SourceFilterConfig {
  if (!sourceId) return DEFAULT_FILTER
  return BY_SOURCE[sourceId] ?? DEFAULT_FILTER
}

export function defaultSortForSource(sourceId?: string | null): string {
  const sorts = sourceFilterConfig(sourceId).sorts
  return sorts[0]?.id ?? ""
}

/** Local filter helper for large tag chip lists. */
export function filterTagOptions(tags: R18FilterOption[], query: string): R18FilterOption[] {
  const q = query.trim().toLowerCase()
  if (!q) return tags
  return tags.filter((item) => {
    if (!item.id) return true
    return item.label.toLowerCase().includes(q) || item.id.toLowerCase().includes(q)
  })
}
