.pragma library
const TEXT_KEYS = ["content", "message", "text", "reply"];
const isChatEvent = (event) => {
  return (event.event_type || event.type || "") === "speak";
};
const normalizeEscapedMarkdown = (value) => {
  const text = value || "";
  if (text.indexOf("\\n```") >= 0 || text.indexOf("\\n~~~") >= 0) {
    return text.replace(/\\r\\n/g, "\n").replace(/\\n/g, "\n").replace(/\\t/g, "	");
  }
  return text;
};
const textFromParsedPayload = (payload) => {
  if (!payload || typeof payload !== "object") {
    return "";
  }
  const obj = payload;
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
const readableEventText = (value) => {
  if (value === void 0 || value === null) {
    return "";
  }
  if (typeof value !== "string") {
    const parsedText = textFromParsedPayload(value);
    return parsedText.length > 0 ? parsedText : JSON.stringify(value);
  }
  const text = value;
  const trimmed = text.trim();
  if (trimmed.length > 0 && (trimmed.charAt(0) === "{" || trimmed.charAt(0) === "[")) {
    try {
      const parsed = JSON.parse(trimmed);
      const extracted = textFromParsedPayload(parsed);
      if (extracted.length > 0) {
        return extracted;
      }
    } catch (error) {
    }
  }
  return normalizeEscapedMarkdown(text);
};
