// Pure helpers for AgentMultiAiChatPane. No QML ids/properties referenced.

interface ChatEvent {
    event_type?: string;
    type?: string;
    [key: string]: unknown;
}

interface ParsedPayload {
    content?: string;
    message?: string;
    text?: string;
    reply?: string;
    payload?: ParsedPayload;
    [key: string]: unknown;
}

const TEXT_KEYS: ReadonlyArray<"content" | "message" | "text" | "reply"> = ["content", "message", "text", "reply"];

export const isChatEvent = (event: ChatEvent): boolean => {
    return (event.event_type || event.type || "") === "speak";
};

export const normalizeEscapedMarkdown = (value: string): string => {
    const text = value || "";
    if (text.indexOf("\\n```") >= 0 || text.indexOf("\\n~~~") >= 0) {
        return text.replace(/\\r\\n/g, "\n").replace(/\\n/g, "\n").replace(/\\t/g, "\t");
    }
    return text;
};

export const textFromParsedPayload = (payload: unknown): string => {
    if (!payload || typeof payload !== "object") {
        return "";
    }
    const obj = payload as ParsedPayload;
    for (let i = 0; i < TEXT_KEYS.length; ++i) {
        const value = obj[TEXT_KEYS[i]];
        if (typeof value === "string" && value.length > 0) {
            return normalizeEscapedMarkdown(value);
        }
    }
    if (obj.payload && typeof obj.payload === "object") {
        return textFromParsedPayload(obj.payload);
    }
    return "";
};

export const readableEventText = (value: unknown): string => {
    if (value === undefined || value === null) {
        return "";
    }
    if (typeof value !== "string") {
        const parsedText = textFromParsedPayload(value);
        return parsedText.length > 0 ? parsedText : JSON.stringify(value);
    }
    const text: string = value;
    const trimmed = text.trim();
    if (trimmed.length > 0 && (trimmed.charAt(0) === "{" || trimmed.charAt(0) === "[")) {
        try {
            const parsed: unknown = JSON.parse(trimmed);
            const extracted = textFromParsedPayload(parsed);
            if (extracted.length > 0) {
                return extracted;
            }
        } catch (error) {
            // Keep the original text when it is not a complete JSON payload.
        }
    }
    return normalizeEscapedMarkdown(text);
};
