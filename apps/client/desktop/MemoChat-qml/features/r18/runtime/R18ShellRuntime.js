.pragma library

function absoluteUrl(url, gatewayBaseUrl) {
    if (!url) {
        return ""
    }
    if (url.indexOf("http://") === 0 || url.indexOf("https://") === 0 || url.indexOf("qrc:/") === 0) {
        return url
    }
    if (url.indexOf("/") === 0 && gatewayBaseUrl.length > 0) {
        return gatewayBaseUrl + url
    }
    return url
}

function normalizeTagArray(tags) {
    if (!tags) {
        return []
    }
    if (typeof tags === "string") {
        return tags.length > 0 ? [tags] : []
    }
    var values = []
    for (var i = 0; i < tags.length; ++i) {
        var tag = String(tags[i] || "").trim()
        if (tag.length > 0) {
            values.push(tag)
        }
    }
    return values
}

function buildSourceTagBuckets(model) {
    if (!model) {
        return []
    }
    var counts = {}
    var total = model.count !== undefined ? model.count : 0
    for (var i = 0; i < total; ++i) {
        var item = model.get(i)
        if (!item) {
            continue
        }
        var tags = normalizeTagArray(item.tags)
        for (var j = 0; j < tags.length; ++j) {
            var key = tags[j]
            if (!counts[key]) {
                counts[key] = 0
            }
            counts[key] += 1
        }
    }
    var buckets = []
    for (var name in counts) {
        buckets.push({ "name": name, "count": counts[name] })
    }
    buckets.sort(function(a, b) {
        if (b.count !== a.count) {
            return b.count - a.count
        }
        return a.name.localeCompare(b.name)
    })
    return buckets
}

function isGridAtBottom(gridView, threshold) {
    if (!gridView) {
        return false
    }
    return gridView.contentHeight <= gridView.height
            || gridView.contentY + gridView.height >= gridView.contentHeight - threshold
}

function modelCount(model) {
    return model && model.count !== undefined ? model.count : 0
}

function currentComicId(currentComic) {
    if (!currentComic) {
        return ""
    }
    return currentComic.comic_id || currentComic.id || ""
}

function currentComicTitle(currentComic) {
    if (!currentComic) {
        return ""
    }
    return currentComic.title || currentComic.name || ""
}
