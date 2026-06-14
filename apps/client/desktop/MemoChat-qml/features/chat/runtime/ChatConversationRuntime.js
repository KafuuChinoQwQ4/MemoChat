.pragma library

// Pure helpers for ChatConversationPane. No QML ids/properties referenced.

function clampValue(value, lower, upper) {
    return Math.max(lower, Math.min(upper, value))
}

// Parse an AI smart-reply result into up to three suggestion strings.
// Accepts a JSON array first, then falls back to line splitting, then pads
// with friendly defaults so callers always get exactly three options.
function parseSuggestionOptions(result) {
    let text = result || ""
    const options = []
    try {
        const parsed = JSON.parse(text)
        if (Array.isArray(parsed)) {
            for (let i = 0; i < parsed.length && options.length < 3; ++i) {
                const item = String(parsed[i]).trim()
                if (item.length > 0) {
                    options.push(item)
                }
            }
        }
    } catch (e) {
    }
    if (options.length === 0) {
        const lines = text.split(/\r?\n/)
        for (let j = 0; j < lines.length && options.length < 3; ++j) {
            let line = lines[j].replace(/^[\s\-*•\d.、)）]+/, "").trim()
            if (line.length > 0) {
                options.push(line)
            }
        }
    }
    while (options.length < 3) {
        options.push(options.length === 0 ? "好的，我看一下。" : (options.length === 1 ? "收到，我稍后回复。" : "可以，我们继续聊。"))
    }
    return options.slice(0, 3)
}
