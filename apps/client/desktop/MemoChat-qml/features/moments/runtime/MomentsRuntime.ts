// Pure helpers shared by MomentsDelegate and MomentsDetailPopup.
// No QML ids/properties referenced — only plain data in, plain data out.

interface MediaItem {
    media_type?: string
    mediaType?: string
    type?: string
    media_key?: string
    mediaKey?: string
    key?: string
    content?: string
    width?: number
    height?: number
}

export const timeAgo = (ts: number): string => {
    if (!ts) return ""
    const nowSec = Math.floor(Date.now() / 1000)
    const diff = nowSec - Math.floor(ts / 1000)
    if (diff < 60) return "刚刚"
    if (diff < 3600) return Math.floor(diff / 60) + "分钟前"
    if (diff < 86400) return Math.floor(diff / 3600) + "小时前"
    if (diff < 604800) return Math.floor(diff / 86400) + "天前"
    return new Date(ts).toLocaleDateString()
}

export const videoDurationText = (durationMs: number): string => {
    if (!durationMs || durationMs <= 0) return "点击打开视频"
    const totalSec = Math.floor(durationMs / 1000)
    const minutes = Math.floor(totalSec / 60)
    const seconds = totalSec % 60
    return minutes + ":" + (seconds < 10 ? "0" + seconds : seconds)
}

export const itemType = (item: MediaItem | null | undefined): string =>
    String((item && (item.media_type || item.mediaType || item.type)) || "").toLowerCase()

export const itemMediaKey = (item: MediaItem | null | undefined): string =>
    String((item && (item.media_key || item.mediaKey || item.key)) || "")

export const itemContent = (item: MediaItem | null | undefined): string =>
    String((item && item.content) || "")

export const isMediaItem = (item: MediaItem | null | undefined): boolean => {
    const type = itemType(item)
    return type === "image" || type === "video"
}

// --- geometry (delegate feed cards) ---

export const imageMaxDim = (item: MediaItem): number => {
    const w = item.width || 200
    const h = item.height || 200
    const aspect = w / (h || 1)
    return Math.min(320, Math.max(80, 200 * aspect))
}

export const imageHeight = (item: MediaItem): number => {
    const w = item.width || 200
    const h = item.height || 200
    const aspect = h / (w || 1)
    return Math.min(240, Math.max(60, 200 * aspect))
}

export const mediaItemHeight = (item: MediaItem): number => {
    if (!isMediaItem(item) || itemMediaKey(item).length === 0) return 0
    if (itemType(item) === "video") return188
    return imageHeight(item)
}

export const mediaContentHeight = (items: MediaItem[] | null | undefined): number => {
    if (!items || items.length === 0) return 0
    let total = 0
    let count = 0
    for (let i = 0; i < items.length; i++) {
        const itemHeight = mediaItemHeight(items[i])
        if (itemHeight <= 0) continue
        if (count > 0) total += 8
        total += itemHeight
        count += 1
    }
    return total
}

export const buildTextContent = (items: MediaItem[] | null | undefined): string => {
    if (!items || items.length === 0) return ""
    const parts: string[] = []
    for (let i = 0; i < items.length; i++) {
        const it = items[i]
        const content = itemContent(it)
        if (content.length > 0 && (!isMediaItem(it) || !itemMediaKey(it))) {
            parts.push(content)
        }
    }
    return parts.join("\n")
}

export const containsMediaContent = (items: MediaItem[] | null | undefined): boolean => {
    if (!items || items.length === 0) return false
    for (let i = 0; i < items.length; i++) {
        const it = items[i]
        if (isMediaItem(it) && itemMediaKey(it).length > 0) return true
    }
    return false
}

// --- geometry (detail popup grid) ---

export const detailImgHeight = (item: MediaItem): number => {
    const w = item.width || 200
    const h = item.height || 200
    return Math.min(160, Math.max(72, 120 * (h / (w || 1))))
}

export const detailMediaTileHeight = (item: MediaItem): number => {
    if (item.type === "video") return 124
    return detailImgHeight(item)
}
