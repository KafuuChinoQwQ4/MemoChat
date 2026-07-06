export const PROVIDERS = {
    openai: {
        display: "OpenAI GPT",
        avatarLabel: "AI",
        color: "#0f9f7a",
        textColor: "#ffffff",
        aliases: ["openai", "gpt", "chatgpt", "oai"],
    },
    claude: {
        display: "Claude",
        avatarLabel: "C",
        color: "#c46b35",
        textColor: "#ffffff",
        aliases: ["anthropic", "claude"],
    },
    gemini: {
        display: "Gemini",
        avatarLabel: "G",
        color: "#4f72d8",
        textColor: "#ffffff",
        aliases: ["google", "gemini", "bard"],
    },
    deepseek: {
        display: "DeepSeek",
        avatarLabel: "D",
        color: "#355fc9",
        textColor: "#ffffff",
        aliases: ["deepseek", "deep-seek"],
    },
    qwen: {
        display: "Qwen",
        avatarLabel: "Q",
        color: "#6a55c8",
        textColor: "#ffffff",
        aliases: ["qwen", "tongyi", "alibaba", "dashscope"],
    },
    glm: {
        display: "GLM",
        avatarLabel: "Z",
        color: "#2f88a7",
        textColor: "#ffffff",
        aliases: ["glm", "zhipu", "chatglm"],
    },
    kimi: {
        display: "Kimi",
        avatarLabel: "K",
        color: "#6f6bd9",
        textColor: "#ffffff",
        aliases: ["kimi", "moonshot"],
    },
    ollama: {
        display: "Ollama",
        avatarLabel: "O",
        color: "#566173",
        textColor: "#ffffff",
        aliases: ["ollama", "local"],
    },
    "custom-api": {
        display: "OpenAI-compatible API",
        avatarLabel: "API",
        color: "#566173",
        textColor: "#ffffff",
        aliases: ["custom-api", "custom_api", "openai-compatible", "compatible", "api"],
    },
};
export const normalize = (value) => {
    if (typeof value === "string")
        return value.toLowerCase().replace(/^\s+|\s+$/g, "");
    if (typeof value === "number" || typeof value === "boolean" || typeof value === "bigint") {
        return String(value).toLowerCase().replace(/^\s+|\s+$/g, "");
    }
    return "";
};
export const providerKey = (modelName, providerId) => {
    const text = normalize(`${providerId || ""} ${modelName || ""}`);
    if (text.length === 0)
        return "custom-api";
    for (const key of Object.keys(PROVIDERS)) {
        if (text === key)
            return key;
        const aliases = PROVIDERS[key]?.aliases ?? [];
        for (const alias of aliases) {
            if (text.includes(alias))
                return key;
        }
    }
    return "custom-api";
};
export const providerMeta = (modelName, providerId) => {
    const key = providerKey(modelName, providerId);
    return PROVIDERS[key] ?? PROVIDERS["custom-api"];
};
export const providerDisplayName = (modelName, providerId) => providerMeta(modelName, providerId).display;
export const providerAvatarDataUrl = (modelName, providerId) => {
    const meta = providerMeta(modelName, providerId);
    const label = meta.avatarLabel;
    const svg = `
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 96 96" role="img" aria-label="${meta.display}">
  <defs>
    <linearGradient id="bg" x1="12" x2="84" y1="10" y2="88" gradientUnits="userSpaceOnUse">
      <stop stop-color="${meta.color}"/>
      <stop offset="1" stop-color="#101827"/>
    </linearGradient>
  </defs>
  <rect width="96" height="96" rx="48" fill="url(#bg)"/>
  <circle cx="48" cy="48" r="31" fill="rgba(255,255,255,.13)" stroke="rgba(255,255,255,.32)" stroke-width="2"/>
  <text x="48" y="56" text-anchor="middle" font-family="Inter, Arial, sans-serif" font-size="${label.length > 1 ? 24 : 34}" font-weight="800" fill="${meta.textColor}">${label}</text>
</svg>`;
    return `data:image/svg+xml,${encodeURIComponent(svg)}`;
};
export const providerPalette = (modelName, providerId) => {
    const key = providerKey(modelName, providerId);
    const meta = providerMeta(modelName, providerId);
    return {
        key,
        display: meta.display,
        color: meta.color,
        textColor: meta.textColor,
        avatar: providerAvatarDataUrl(modelName, providerId),
    };
};
