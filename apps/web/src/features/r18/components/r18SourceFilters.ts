/** Per-source sort / category / tag filter options for R18 browse UI. */

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

const JM_SORTS: R18FilterOption[] = [
  { id: "mr", label: "最新发布" },
  { id: "mv_t", label: "今日最火" },
  { id: "mv_w", label: "本周最火" },
  { id: "mv_m", label: "本月最火" },
  { id: "mv", label: "历史最火" },
  { id: "mp", label: "点赞最多" },
  { id: "ld", label: "爱心最多" },
  { id: "tf", label: "图片最多" },
]

/** JM 官方常见分类 / 主题标签（搜索关键词即 tag）。 */
const JM_TAGS: R18FilterOption[] = [
  { id: "", label: "全部" },
  // 官方主分类
  { id: "同人", label: "同人" },
  { id: "单行本", label: "单行本" },
  { id: "短篇", label: "短篇" },
  { id: "其他", label: "其他" },
  { id: "韩漫", label: "韩漫" },
  { id: "美漫", label: "美漫" },
  { id: "Cosplay", label: "Cosplay" },
  { id: "3D", label: "3D" },
  { id: "CG", label: "CG" },
  // 语言 / 制作
  { id: "中文", label: "中文" },
  { id: "汉化", label: "汉化" },
  { id: "日语", label: "日语" },
  { id: "英文", label: "英文" },
  { id: "全彩", label: "全彩" },
  { id: "黑白", label: "黑白" },
  { id: "无码", label: "无码" },
  { id: "有码", label: "有码" },
  { id: "生肉", label: "生肉" },
  // 常见题材
  { id: "巨乳", label: "巨乳" },
  { id: "贫乳", label: "贫乳" },
  { id: "美少女", label: "美少女" },
  { id: "御姐", label: "御姐" },
  { id: "熟女", label: "熟女" },
  { id: "人妻", label: "人妻" },
  { id: "师生", label: "师生" },
  { id: "姐妹", label: "姐妹" },
  { id: "母女", label: "母女" },
  { id: "NTR", label: "NTR" },
  { id: "出轨", label: "出轨" },
  { id: "纯爱", label: "纯爱" },
  { id: "恋爱", label: "恋爱" },
  { id: "后宫", label: "后宫" },
  { id: "百合", label: "百合" },
  { id: "耽美", label: "耽美" },
  { id: "伪娘", label: "伪娘" },
  { id: "扶他", label: "扶他" },
  { id: "女装", label: "女装" },
  { id: "性转", label: "性转" },
  { id: "群P", label: "群P" },
  { id: "乱伦", label: "乱伦" },
  { id: "调教", label: "调教" },
  { id: "束缚", label: "束缚" },
  { id: "重口", label: "重口" },
  { id: "猎奇", label: "猎奇" },
  { id: "触手", label: "触手" },
  { id: "异种奸", label: "异种奸" },
  { id: "怪物", label: "怪物" },
  { id: "悬疑", label: "悬疑" },
  { id: "剧情", label: "剧情" },
  { id: "校园", label: "校园" },
  { id: "职场", label: "职场" },
  { id: "奇幻", label: "奇幻" },
  { id: "科幻", label: "科幻" },
  { id: "末日", label: "末日" },
  { id: "游戏改", label: "游戏改" },
  { id: "动画改", label: "动画改" },
  { id: "偶像", label: "偶像" },
  { id: "女同", label: "女同" },
  { id: "男同", label: "男同" },
  { id: "SM", label: "SM" },
  { id: "露出", label: "露出" },
  { id: "催眠", label: "催眠" },
  { id: "强制", label: "强制" },
  { id: "凌辱", label: "凌辱" },
  { id: "泳装", label: "泳装" },
  { id: "制服", label: "制服" },
  { id: "女仆", label: "女仆" },
  { id: "护士", label: "护士" },
  { id: "丝袜", label: "丝袜" },
  { id: "足交", label: "足交" },
  { id: "口交", label: "口交" },
  { id: "肛交", label: "肛交" },
  { id: "中出", label: "中出" },
  { id: "内射", label: "内射" },
  { id: "颜射", label: "颜射" },
  { id: "怀孕", label: "怀孕" },
  { id: "产卵", label: "产卵" },
  { id: "阿黑颜", label: "阿黑颜" },
  { id: "眼睛", label: "眼睛" },
  { id: "眼镜", label: "眼镜" },
  { id: "双马尾", label: "双马尾" },
  { id: "长发", label: "长发" },
  { id: "短发", label: "短发" },
]

const PICACG_SORTS: R18FilterOption[] = [
  { id: "dd", label: "最新发布" },
  { id: "da", label: "最早发布" },
  { id: "ld", label: "点赞最多" },
  { id: "vd", label: "最多指名" },
]

/** Picacg 常见分区 / 标签关键词。 */
const PICACG_TAGS: R18FilterOption[] = [
  { id: "", label: "全部" },
  { id: "全彩", label: "全彩" },
  { id: "長篇", label: "长篇" },
  { id: "短篇", label: "短篇" },
  { id: "完結", label: "完结" },
  { id: "同人", label: "同人" },
  { id: "單行本", label: "单行本" },
  { id: "韓漫", label: "韩漫" },
  { id: "美漫", label: "美漫" },
  { id: "日漫", label: "日漫" },
  { id: "CG雜圖", label: "CG杂图" },
  { id: "生肉", label: "生肉" },
  { id: "漢化", label: "汉化" },
  { id: "純愛", label: "纯爱" },
  { id: "戀愛", label: "恋爱" },
  { id: "後宮", label: "后宫" },
  { id: "百合", label: "百合" },
  { id: "耽美", label: "耽美" },
  { id: "偽娘", label: "伪娘" },
  { id: "扶他", label: "扶他" },
  { id: "性轉", label: "性转" },
  { id: "NTR", label: "NTR" },
  { id: "重口", label: "重口" },
  { id: "獵奇", label: "猎奇" },
  { id: "溫馨", label: "温馨" },
  { id: "劇情", label: "剧情" },
  { id: "校園", label: "校园" },
  { id: "幻想", label: "幻想" },
  { id: "科幻", label: "科幻" },
  { id: "戰鬥", label: "战斗" },
  { id: "懸疑", label: "悬疑" },
  { id: "歷史", label: "历史" },
  { id: "其他", label: "其他" },
  { id: "紳仕", label: "绅士" },
  { id: "巨乳", label: "巨乳" },
  { id: "貧乳", label: "贫乳" },
  { id: "人妻", label: "人妻" },
  { id: "教師", label: "教师" },
  { id: "姊妹", label: "姊妹" },
  { id: "母女", label: "母女" },
  { id: "調教", label: "调教" },
  { id: "束縛", label: "束缚" },
  { id: "觸手", label: "触手" },
  { id: "異種", label: "异种" },
  { id: "怪物", label: "怪物" },
  { id: "機械", label: "机械" },
  { id: "群P", label: "群P" },
  { id: "亂倫", label: "乱伦" },
  { id: "露出", label: "露出" },
  { id: "催眠", label: "催眠" },
  { id: "強制", label: "强制" },
  { id: "凌辱", label: "凌辱" },
  { id: "SM", label: "SM" },
  { id: "足控", label: "足控" },
  { id: "眼鏡", label: "眼镜" },
  { id: "制服", label: "制服" },
  { id: "泳裝", label: "泳装" },
  { id: "絲襪", label: "丝袜" },
  { id: "中出", label: "中出" },
  { id: "懷孕", label: "怀孕" },
]

const NHENTAI_SORTS: R18FilterOption[] = [
  { id: "", label: "最新发布" },
  { id: "popular-today", label: "今日最火" },
  { id: "popular-week", label: "本周最火" },
  { id: "popular-month", label: "本月最火" },
  { id: "popular", label: "历史最火" },
]

/** nHentai 语言 + 高频官方 tag。 */
const NHENTAI_TAGS: R18FilterOption[] = [
  { id: "", label: "全部" },
  // language
  { id: "language:chinese", label: "中文" },
  { id: "language:japanese", label: "日文" },
  { id: "language:english", label: "英文" },
  { id: "language:translated", label: "翻译" },
  { id: "language:speechless", label: "无对白" },
  { id: "language:text cleaned", label: "去字版" },
  // high-frequency tags
  { id: "full color", label: "全彩" },
  { id: "big breasts", label: "巨乳" },
  { id: "small breasts", label: "贫乳" },
  { id: "sole female", label: "单女主" },
  { id: "sole male", label: "单男主" },
  { id: "group", label: "群P" },
  { id: "milf", label: "熟女" },
  { id: "schoolgirl uniform", label: "学生制服" },
  { id: "stockings", label: "丝袜" },
  { id: "glasses", label: "眼镜" },
  { id: "nakadashi", label: "中出" },
  { id: "ahegao", label: "阿黑颜" },
  { id: "blowjob", label: "口交" },
  { id: "anal", label: "肛交" },
  { id: "ffm threesome", label: "FFM 3P" },
  { id: "mmf threesome", label: "MMF 3P" },
  { id: "yuri", label: "百合" },
  { id: "yaoi", label: "耽美" },
  { id: "futanari", label: "扶他" },
  { id: "crossdressing", label: "女装" },
  { id: "gender bender", label: "性转" },
  { id: "netorare", label: "NTR" },
  { id: "cheating", label: "出轨" },
  { id: "incest", label: "乱伦" },
  { id: "mind control", label: "精神控制" },
  { id: "rape", label: "强制" },
  { id: "bondage", label: "束缚" },
  { id: "bdsm", label: "BDSM" },
  { id: "tentacles", label: "触手" },
  { id: "monster", label: "怪物" },
  { id: "elf", label: "精灵" },
  { id: "demon girl", label: "魅魔/魔娘" },
  { id: "dark skin", label: "黑皮" },
  { id: "blonde hair", label: "金发" },
  { id: "twintails", label: "双马尾" },
  { id: "ponytail", label: "马尾" },
  { id: "mosaic censorship", label: "有码" },
  { id: "uncensored", label: "无码" },
  { id: "multi-work series", label: "系列作" },
  { id: "anthology", label: "合集" },
  { id: "doujinshi", label: "同人志" },
  { id: "manga", label: "漫画" },
  { id: "western", label: "西方" },
  { id: "non-h", label: "Non-H" },
  { id: "artist cg", label: "Artist CG" },
  { id: "game cg", label: "Game CG" },
  { id: "cosplay", label: "Cosplay" },
  { id: "imageset", label: "图集" },
]

const EHENTAI_SORTS: R18FilterOption[] = [
  { id: "", label: "全部分类" },
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

/** e-hentai 常用 namespace:tag。 */
const EHENTAI_TAGS: R18FilterOption[] = [
  { id: "", label: "无附加 tag" },
  { id: "language:chinese", label: "中文" },
  { id: "language:japanese", label: "日文" },
  { id: "language:english", label: "英文" },
  { id: "language:translated", label: "翻译" },
  { id: "language:speechless", label: "无对白" },
  { id: "language:text cleaned", label: "去字版" },
  { id: "other:full color", label: "全彩" },
  { id: "other:mosaic censorship", label: "有码" },
  { id: "other:uncensored", label: "无码" },
  { id: "female:big breasts", label: "巨乳" },
  { id: "female:small breasts", label: "贫乳" },
  { id: "female:sole female", label: "单女主" },
  { id: "male:sole male", label: "单男主" },
  { id: "female:milf", label: "熟女" },
  { id: "female:schoolgirl uniform", label: "学生制服" },
  { id: "female:stockings", label: "丝袜" },
  { id: "female:glasses", label: "眼镜" },
  { id: "female:nakadashi", label: "中出" },
  { id: "female:ahegao", label: "阿黑颜" },
  { id: "female:blowjob", label: "口交" },
  { id: "female:anal", label: "肛交" },
  { id: "female:group", label: "女群P" },
  { id: "male:group", label: "男群P" },
  { id: "female:yuri", label: "百合" },
  { id: "male:yaoi", label: "耽美" },
  { id: "female:futanari", label: "扶他" },
  { id: "male:crossdressing", label: "男女装" },
  { id: "female:gender bender", label: "性转" },
  { id: "female:netorare", label: "NTR" },
  { id: "female:cheating", label: "出轨" },
  { id: "female:incest", label: "乱伦" },
  { id: "female:mind control", label: "精神控制" },
  { id: "female:rape", label: "强制" },
  { id: "female:bondage", label: "束缚" },
  { id: "female:bdsm", label: "BDSM" },
  { id: "female:tentacles", label: "触手" },
  { id: "female:monster", label: "怪物" },
  { id: "female:elf", label: "精灵" },
  { id: "female:demon girl", label: "魔娘" },
  { id: "female:dark skin", label: "黑皮" },
  { id: "female:blonde hair", label: "金发" },
  { id: "female:twintails", label: "双马尾" },
  { id: "female:ponytail", label: "马尾" },
  { id: "female:multi-work series", label: "系列作" },
  { id: "other:anthology", label: "合集" },
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
    tagPlaceholder: "自定义分类 / tag，例如 同人、全彩",
    allowCustomTag: true,
  },
  "picacg.official": {
    sorts: PICACG_SORTS,
    tags: PICACG_TAGS,
    tagPlaceholder: "自定义分类关键词，例如 全彩",
    allowCustomTag: true,
  },
  "nhentai.official": {
    sorts: NHENTAI_SORTS,
    tags: NHENTAI_TAGS,
    tagPlaceholder: "自定义 tag，例如 sole female",
    allowCustomTag: true,
  },
  "ehentai.official": {
    // On e-hentai the "sort" chips are category filters (f_cats).
    sorts: EHENTAI_SORTS,
    tags: EHENTAI_TAGS,
    tagPlaceholder: "附加搜索 tag，例如 language:chinese",
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
