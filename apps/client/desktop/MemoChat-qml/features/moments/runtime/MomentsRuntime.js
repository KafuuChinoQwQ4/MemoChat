.pragma library

// Pure helpers shared by MomentsDelegate and MomentsDetailPopup.
// No QML ids/properties referenced — only plain data in, plain data out.

function timeAgo(ts) {
    if (!ts)
        return ""
    var nowSec = Math.floor(Date.now() / 1000)
    var diff = nowSec - Math.floor(ts / 1000)
    if (diff < 60)
        return "刚刚"
    if (diff < 3600)
        return Math.floor(diff / 60) + "分钟前"
    if (diff < 86400)
        return Math.floor(diff / 3600) + "小时前"
    if (diff < 604800)
        return Math.floor(diff / 86400) + "天前"
    return new Date(ts).toLocaleDateString()
}

function videoDurationText(durationMs) {
    if (!durationMs || durationMs <= 0)
        return "点击打开视频"
    var totalSec = Math.floor(durationMs / 1000)
    var minutes = Math.floor(totalSec / 60)
    var seconds = totalSec % 60
    return minutes + ":" + (seconds < 10 ? "0" + seconds : seconds)
}

function itemType(item) {
    return String((item && (item.media_type || item.mediaType || item.type)) || "").toLowerCase()
}

function itemMediaKey(item) {
    return String((item && (item.media_key || item.mediaKey || item.key)) || "")
}

function itemContent(item) {
    return String((item && item.content) || "")
}

function isMediaItem(item) {
    var type = itemType(item)
    return type === "image" || type === "video"
}

// --- geometry (delegate feed cards) ---

function imageMaxDim(item) {
    var w = item.width || 200
    var h = item.height || 200
    var aspect = w / (h || 1)
    return Math.min(320, Math.max(80, 200 * aspect))
}

function imageHeight(item) {
    var w = item.width || 200
    var h = item.height || 200
    var aspect = h / (w || 1)
    return Math.min(240, Math.max(60, 200 * aspect))
}

function mediaItemHeight(item) {
    if (!isMediaItem(item) || itemMediaKey(item).length === 0)
        return 0
    if (itemType(item) === "video")
        return 188
    return imageHeight(item)
}

function mediaContentHeight(items) {
    if (!items || items.length === 0)
        return 0
    var total = 0
    var count = 0
    for (var i = 0; i < items.length; i++) {
        var itemHeight = mediaItemHeight(items[i])
        if (itemHeight <= 0)
            continue
        if (count > 0)
            total += 8
        total += itemHeight
        count += 1
    }
    return total
}

function buildTextContent(items) {
    if (!items || items.length === 0)
        return ""
    var parts = []
    for (var i = 0; i < items.length; i++) {
        var it = items[i]
        var content = itemContent(it)
        if (content.length > 0 && (!isMediaItem(it) || !itemMediaKey(it))) {
            parts.push(content)
        }
    }
    return parts.join("\n")
}

function containsMediaContent(items) {
    if (!items || items.length === 0)
        return false
    for (var i = 0; i < items.length; i++) {
        var it = items[i]
        if (isMediaItem(it) && itemMediaKey(it).length > 0) {
            return true
        }
    }
    return false
}

// --- geometry (detail popup grid) ---

function detailImgHeight(item) {
    var w = item.width || 200, h = item.height || 200
    return Math.min(160, Math.max(72, 120 * (h / (w || 1))))
}

function detailMediaTileHeight(item) {
    if (item.type === "video")
        return 124
    return detailImgHeight(item)
}

