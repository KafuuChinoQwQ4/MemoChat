interface ProviderMeta {
    display: string;
    avatar: string;
    color: string;aliases: string[];
}

interface ProviderPalette {
    key: string;
    display: string;
    color: string;
    avatar: string;
}

export const PROVIDERS: Record<string, ProviderMeta> = {
    "openai":{ display: "OpenAI GPT",avatar: "qrc:/icons/ai_provider_openai.svg",   color: "#0f9f7a", aliases: ["openai", "gpt", "chatgpt", "oai"] },
    "claude":      { display: "Claude",                avatar: "qrc:/icons/ai_provider_claude.svg",   color: "#c46b35", aliases: ["anthropic", "claude"] },
    "gemini":      { display: "Gemini",                avatar: "qrc:/icons/ai_provider_gemini.svg",   color: "#4f72d8", aliases: ["google", "gemini", "bard"] },
    "deepseek":    { display: "DeepSeek",              avatar: "qrc:/icons/ai_provider_deepseek.svg", color: "#355fc9", aliases: ["deepseek", "deep-seek"] },
    "qwen":        { display: "Qwen",                  avatar: "qrc:/icons/ai_provider_qwen.svg",     color: "#6a55c8", aliases: ["qwen", "tongyi", "alibaba", "dashscope"] },
    "glm":         { display: "GLM",                   avatar: "qrc:/icons/ai_provider_glm.svg",      color: "#2f88a7", aliases: ["glm", "zhipu", "chatglm"] },
    "kimi":        { display: "Kimi",                  avatar: "qrc:/icons/ai_provider_kimi.svg",     color: "#6f6bd9", aliases: ["kimi", "moonshot"] },
    "ollama":      { display: "Ollama",                avatar: "qrc:/icons/ai_provider_generic.svg",  color: "#566173", aliases: ["ollama", "local"] },
    "custom-api":  { display: "OpenAI-compatible API", avatar: "qrc:/icons/ai_provider_generic.svg",  color: "#566173", aliases: ["custom-api", "custom_api", "openai-compatible", "compatible", "api"] },
};

export const normalize = (value: unknown): string =>
    (value || "").toString().toLowerCase().replace(/^\s+|\s+$/g, "");

export const providerKey = (modelName: string | null | undefined, providerId: string | null | undefined): string => {
    const text = normalize((providerId || "") + " " + (modelName || ""));
    if (text.length === 0) { return "custom-api"; }
    const keys = Object.keys(PROVIDERS);
    for (let i = 0; i < keys.length; ++i) {
        const key = keys[i];
        if (text === key) { return key; }const aliases: string[] = PROVIDERS[key].aliases || [];
        for (let j = 0; j < aliases.length; ++j) {
            if (text.indexOf(aliases[j]) >= 0) { return key; }
        }
    }
    return "custom-api";
};

export const providerMeta = (modelName: string | null | undefined, providerId: string | null | undefined): ProviderMeta => {
    const key = providerKey(modelName, providerId);
    return PROVIDERS[key] || PROVIDERS["custom-api"];
};

export const providerDisplayName = (modelName: string | null | undefined, providerId: string | null | undefined): string =>
    providerMeta(modelName, providerId).display;

export const providerAvatarSource = (modelName: string | null | undefined, providerId: string | null | undefined): string =>
    providerMeta(modelName, providerId).avatar;

export const providerPalette = (modelName: string | null | undefined, providerId: string | null | undefined): ProviderPalette => {
    const meta = providerMeta(modelName, providerId);
    return { key: providerKey(modelName, providerId), display: meta.display, color: meta.color, avatar: meta.avatar };
};
