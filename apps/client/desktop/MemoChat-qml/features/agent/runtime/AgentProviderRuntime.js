.pragma library

var PROVIDERS = {
    "openai": {
        "display": "OpenAI GPT",
        "avatar": "qrc:/icons/ai_provider_openai.svg",
        "color": "#0f9f7a",
        "aliases": ["openai", "gpt", "chatgpt", "oai"]
    },
    "claude": {
        "display": "Claude",
        "avatar": "qrc:/icons/ai_provider_claude.svg",
        "color": "#c46b35",
        "aliases": ["anthropic", "claude"]
    },
    "gemini": {
        "display": "Gemini",
        "avatar": "qrc:/icons/ai_provider_gemini.svg",
        "color": "#4f72d8",
        "aliases": ["google", "gemini", "bard"]
    },
    "deepseek": {
        "display": "DeepSeek",
        "avatar": "qrc:/icons/ai_provider_deepseek.svg",
        "color": "#355fc9",
        "aliases": ["deepseek", "deep-seek"]
    },
    "qwen": {
        "display": "Qwen",
        "avatar": "qrc:/icons/ai_provider_qwen.svg",
        "color": "#6a55c8",
        "aliases": ["qwen", "tongyi", "alibaba", "dashscope"]
    },
    "glm": {
        "display": "GLM",
        "avatar": "qrc:/icons/ai_provider_glm.svg",
        "color": "#2f88a7",
        "aliases": ["glm", "zhipu", "chatglm"]
    },
    "kimi": {
        "display": "Kimi",
        "avatar": "qrc:/icons/ai_provider_kimi.svg",
        "color": "#6f6bd9",
        "aliases": ["kimi", "moonshot"]
    },
    "ollama": {
        "display": "Ollama",
        "avatar": "qrc:/icons/ai_provider_generic.svg",
        "color": "#566173",
        "aliases": ["ollama", "local"]
    },
    "custom-api": {
        "display": "OpenAI-compatible API",
        "avatar": "qrc:/icons/ai_provider_generic.svg",
        "color": "#566173",
        "aliases": ["custom-api", "custom_api", "openai-compatible", "compatible", "api"]
    }
}

function normalize(value) {
    return (value || "").toString().toLowerCase().replace(/^\s+|\s+$/g, "")
}

function providerKey(modelName, providerId) {
    var text = normalize((providerId || "") + " " + (modelName || ""))
    if (text.length === 0) {
        return "custom-api"
    }
    var keys = Object.keys(PROVIDERS)
    for (var i = 0; i < keys.length; ++i) {
        var key = keys[i]
        if (text === key) {
            return key
        }
        var aliases = PROVIDERS[key].aliases || []
        for (var j = 0; j < aliases.length; ++j) {
            if (text.indexOf(aliases[j]) >= 0) {
                return key
            }
        }
    }
    return "custom-api"
}

function providerMeta(modelName, providerId) {
    var key = providerKey(modelName, providerId)
    return PROVIDERS[key] || PROVIDERS["custom-api"]
}

function providerDisplayName(modelName, providerId) {
    return providerMeta(modelName, providerId).display
}

function providerAvatarSource(modelName, providerId) {
    return providerMeta(modelName, providerId).avatar
}

function providerPalette(modelName, providerId) {
    var meta = providerMeta(modelName, providerId)
    return {
        "key": providerKey(modelName, providerId),
        "display": meta.display,
        "color": meta.color,
        "avatar": meta.avatar
    }
}
