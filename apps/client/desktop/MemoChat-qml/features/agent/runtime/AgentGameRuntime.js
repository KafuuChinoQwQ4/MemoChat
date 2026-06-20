.pragma library

function defaultModel() {
    return { "model_type": "ollama", "model_name": "qwen2.5:7b" }
}

function firstModel(availableModels) {
    if (availableModels && availableModels.length > 0) {
        return availableModels[0]
    }
    return defaultModel()
}

function modelLabel(model) {
    if (!model) {
        model = defaultModel()
    }
    return String(model.model_type || "") + ":" + String(model.model_name || "")
}

function formalRulesets(gameRulesets, testRulesetId) {
    var rows = []
    var preferred = null
    for (var i = 0; i < (gameRulesets ? gameRulesets.length : 0); ++i) {
        var item = gameRulesets[i] || {}
        var rulesetId = item.ruleset_id || item.id || ""
        if (rulesetId !== testRulesetId) {
            if (rulesetId === "werewolf.basic") {
                preferred = item
            } else {
                rows.push(item)
            }
        }
    }
    if (preferred !== null) {
        rows.unshift(preferred)
    }
    if (rows.length === 0) {
        rows.push({
            "ruleset_id": "werewolf.basic",
            "display_name": "狼人杀"
        })
    }
    return rows
}

function fallbackWerewolfRoles() {
    return [
        {
            "role_key": "werewolf",
            "display_name": "狼人",
            "description": "夜晚可选择击杀目标；白天隐藏身份、误导投票，狼人阵营胜利条件是人数压制好人阵营。",
            "persona": "你是狼人。发言克制，保护自己，制造合理怀疑。"
        },
        {
            "role_key": "villager",
            "display_name": "村民",
            "description": "没有夜晚技能；白天通过发言、票型和逻辑找出狼人。",
            "persona": "你是村民。基于时间线和发言矛盾做判断。"
        },
        {
            "role_key": "seer",
            "display_name": "预言家",
            "description": "每晚可查验一名玩家的阵营；白天需要在保护身份和公布信息之间取舍。",
            "persona": "你是预言家。谨慎利用查验信息带队。"
        },
        {
            "role_key": "witch",
            "display_name": "女巫",
            "description": "拥有解药和毒药；夜晚可救人或毒杀一名玩家，通常每种药只能使用一次。",
            "persona": "你是女巫。谨慎管理药水并观察狼人刀口。"
        },
        {
            "role_key": "hunter",
            "display_name": "猎人",
            "description": "出局时可开枪带走一名玩家；需要用发言压制狼人并保护好人信息。",
            "persona": "你是猎人。保持威慑，避免被狼人轻易带偏。"
        },
        {
            "role_key": "guard",
            "display_name": "守卫",
            "description": "每晚可守护一名玩家免于出局；通常不能连续守护同一目标。",
            "persona": "你是守卫。根据局势选择保护目标。"
        },
        {
            "role_key": "idiot",
            "display_name": "白痴",
            "description": "被投票出局时可翻牌免死，但之后通常失去投票权，只能继续发言。",
            "persona": "你是白痴。用稳定发言吸收压力并提供判断。"
        }
    ]
}

function roleOptions(rolePresets, rulesetId, customValue, randomHint, randomRule, customRule) {
    var rows = [{
        "label": "系统随机",
        "value": "",
        "hint": randomHint,
        "rule": randomRule,
        "showValue": false
    }]
    var seen = { "": true }
    var source = rolePresets && rolePresets.length > 0
        ? rolePresets
        : (rulesetId === "werewolf.basic" ? fallbackWerewolfRoles() : [])
    for (var i = 0; i < source.length; ++i) {
        var item = source[i] || {}
        var key = item.role_key || item.key || ""
        if (key.length === 0 || seen[key]) {
            continue
        }
        rows.push({
            "label": item.display_name || key,
            "value": key,
            "hint": item.description || item.rule || item.persona || "",
            "rule": item.rule || item.rules || item.description || item.persona || "",
            "showValue": false
        })
        seen[key] = true
    }
    rows.push({
        "label": "自定义身份",
        "value": customValue,
        "hint": "输入自定义 role_key",
        "rule": customRule,
        "showValue": false
    })
    return rows
}

function skillOptions(customSkillValue) {
    return [
        { "label": "写作", "value": "writer", "hint": "生成自然发言和可读文本" },
        { "label": "通用对话", "value": "general_chat", "hint": "轻量聊天和普通回复" },
        { "label": "研究", "value": "researcher", "hint": "证据收集和分析" },
        { "label": "审阅", "value": "reviewer", "hint": "检查风险和漏洞" },
        { "label": "支持", "value": "support", "hint": "排查问题和给步骤" },
        { "label": "知识整理", "value": "knowledge_curator", "hint": "整理知识和摘要" },
        { "label": "知识库问答", "value": "knowledge_copilot", "hint": "优先检索知识库" },
        { "label": "联网研究", "value": "research_assistant", "hint": "搜索后回答" },
        { "label": "图谱推荐", "value": "graph_recommender", "hint": "使用关系图谱" },
        { "label": "自定义 skill", "value": customSkillValue, "hint": "输入后端已支持的新 skill" }
    ]
}

function strategyOptions(customStrategyValue) {
    return [
        { "label": "角色扮演", "value": "roleplay", "hint": "优先保持人设和口吻" },
        { "label": "均衡", "value": "balanced", "hint": "稳定中立的发言方式" },
        { "label": "群聊", "value": "group_chat", "hint": "适合多 AI 聊天" },
        { "label": "推理", "value": "deductive", "hint": "重视线索和逻辑" },
        { "label": "观察", "value": "observant", "hint": "关注细节和他人发言" },
        { "label": "调查", "value": "investigative", "hint": "主动追问和求证" },
        { "label": "强势", "value": "assertive", "hint": "表达明确，推动局面" },
        { "label": "进攻", "value": "aggressive", "hint": "更主动施压" },
        { "label": "欺骗", "value": "deceptive", "hint": "适合隐藏身份角色" },
        { "label": "直觉", "value": "intuitive", "hint": "更依赖感觉和判断" },
        { "label": "自定义风格", "value": customStrategyValue, "hint": "输入自定义 strategy" }
    ]
}

function optionIndex(options, value, fallbackValue) {
    var current = value && value.length > 0 ? value : fallbackValue
    for (var i = 0; i < options.length; ++i) {
        if (options[i].value === current) {
            return i
        }
    }
    return Math.max(0, options.length - 1)
}

function containsValue(values, value) {
    for (var i = 0; i < values.length; ++i) {
        if (values[i] === value) {
            return true
        }
    }
    return false
}

function optionText(options, index, customValues) {
    if (index < 0 || index >= options.length) {
        return ""
    }
    var item = options[index] || {}
    var value = item.value || ""
    if (item.showValue === false || value.length === 0 || containsValue(customValues || [], value)) {
        return item.label
    }
    return item.label + " · " + value
}

function optionRule(options, index) {
    if (index < 0 || index >= options.length) {
        return ""
    }
    var item = options[index] || {}
    return item.rule || item.hint || ""
}

function isCustomStoredValue(options, value) {
    if (!value || value.length === 0) {
        return false
    }
    for (var i = 0; i < options.length; ++i) {
        if (options[i].value === value) {
            return false
        }
    }
    return true
}

function createDraftAgent(seed, index, gameSetupMode, model) {
    var item = seed || {}
    model = model || defaultModel()
    return {
        "display_name": item.display_name || ((gameSetupMode ? "玩家 AI " : "AI ") + (index + 1)),
        "role_key": gameSetupMode ? (item.role_key || "") : "",
        "environment": item.environment || "",
        "persona": item.persona || "",
        "skill_name": item.skill_name || "writer",
        "strategy": item.strategy || (gameSetupMode ? "roleplay" : "group_chat"),
        "model_type": item.model_type || model.model_type || "ollama",
        "model_name": item.model_name || model.model_name || "qwen2.5:7b"
    }
}

function normalizedAgent(agent, model, gameSetupMode, gameContent) {
    model = model || defaultModel()
    agent = agent || {}
    var env = agent.environment || ""
    var persona = agent.persona || ""
    if (env.length > 0) {
        persona = "上下文:\n" + env + (persona.length > 0 ? "\n\n说话风格:\n" + persona : "")
    }
    if (gameContent.length > 0) {
        persona = "游戏内容:\n" + gameContent + (persona.length > 0 ? "\n\n角色设定:\n" + persona : "")
    }
    return {
        "display_name": agent.display_name || "AI",
        "role_key": gameSetupMode ? (agent.role_key || "") : "",
        "persona": persona,
        "skill_name": agent.skill_name || "writer",
        "strategy": agent.strategy || (gameSetupMode ? "roleplay" : "group_chat"),
        "model_type": agent.model_type || model.model_type || "ollama",
        "model_name": agent.model_name || model.model_name || "qwen2.5:7b"
    }
}

function disabledHostConfig(humanRoleKey) {
    return {
        "enabled": false,
        "display_name": "",
        "persona": "",
        "model_type": "",
        "model_name": "",
        "skill_name": "",
        "human_role_key": humanRoleKey || ""
    }
}

function hostConfig(gameSetupMode, hostEnabled, hostName, hostPersona, gameContent, model, humanRoleKey) {
    if (!gameSetupMode || !hostEnabled) {
        return disabledHostConfig(gameSetupMode ? humanRoleKey : "")
    }
    model = model || defaultModel()
    return {
        "enabled": true,
        "display_name": hostName.length > 0 ? hostName : "游戏主持人",
        "persona": hostPersona.length > 0 ? hostPersona : gameContent,
        "model_type": model.model_type || "ollama",
        "model_name": model.model_name || "qwen2.5:7b",
        "skill_name": "writer",
        "human_role_key": humanRoleKey
    }
}
