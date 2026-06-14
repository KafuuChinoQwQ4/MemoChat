.pragma library

// Pure helpers for AgentMultiAiChatPane. No QML ids/properties referenced.

function isChatEvent(event) {
    return (event.event_type || event.type || "") === "speak"
}

function normalizeEscapedMarkdown(value) {
    var text = value || ""
    if (text.indexOf("\\n```") >= 0 || text.indexOf("\\n~~~") >= 0) {
        return text.replace(/\\r\\n/g, "\n").replace(/\\n/g, "\n").replace(/\\t/g, "\t")
    }
    return text
}

function textFromParsedPayload(payload) {
    if (!payload || typeof payload !== "object") {
        return ""
    }
    var keys = ["content", "message", "text", "reply"]
    for (var i = 0; i < keys.length; ++i) {
        var value = payload[keys[i]]
        if (typeof value === "string" && value.length > 0) {
            return normalizeEscapedMarkdown(value)
        }
    }
    if (payload.payload && typeof payload.payload === "object") {
        return textFromParsedPayload(payload.payload)
    }
    return ""
}

function readableEventText(value) {
    if (value === undefined || value === null) {
        return ""
    }
    if (typeof value !== "string") {
        var parsedText = textFromParsedPayload(value)
        return parsedText.length > 0 ? parsedText : JSON.stringify(value)
    }
    var text = value
    var trimmed = text.trim()
    if (trimmed.length > 0 && (trimmed.charAt(0) === "{" || trimmed.charAt(0) === "[")) {
        try {
            var parsed = JSON.parse(trimmed)
            var extracted = textFromParsedPayload(parsed)
            if (extracted.length > 0) {
                return extracted
            }
        } catch (error) {
            // Keep the original text when it is not a complete JSON payload.
        }
    }
    return normalizeEscapedMarkdown(text)
}
