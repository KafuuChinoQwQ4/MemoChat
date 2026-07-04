export interface ModelConfig {
  model_type: string
  model_name: string
}

export interface RulesetItem {
  ruleset_id?: string
  id?: string
  display_name?: string
  [key: string]: unknown
}

export interface RoleItem {
  role_key?: string
  key?: string
  display_name?: string
  description?: string
  persona?: string
  rule?: string
  rules?: string
  [key: string]: unknown
}

export interface OptionItem {
  label: string
  value: string
  hint: string
  rule?: string
  showValue: boolean
}

export interface SkillOption {
  label: string
  value: string
  hint: string
}

export interface StrategyOption {
  label: string
  value: string
  hint: string
}

export interface DraftAgent {
  display_name: string
  role_key: string
  environment: string
  persona: string
  skill_name: string
  strategy: string
  model_type: string
  model_name: string
}

export interface NormalizedAgent {
  display_name: string
  role_key: string
  persona: string
  skill_name: string
  strategy: string
  model_type: string
  model_name: string
}

export interface HostConfig {
  enabled: boolean
  display_name: string
  persona: string
  model_type: string
  model_name: string
  skill_name: string
  human_role_key: string
}

export const defaultModel = (): ModelConfig => ({
  model_type: "ollama",
  model_name: "qwen2.5:7b",
})

export const firstModel = (availableModels: ModelConfig[] | null | undefined): ModelConfig => {
  if (availableModels && availableModels.length > 0) {
    return availableModels[0]
  }
  return defaultModel()
}

export const modelLabel = (model: ModelConfig | null | undefined): string => {
  let m = model
  if (!m) {
    m = defaultModel()
  }
  return String(m.model_type || "") + ":" + String(m.model_name || "")
}

export const formalRulesets = (
  gameRulesets: RulesetItem[] | null | undefined,
  testRulesetId: string
): RulesetItem[] => {
  const rows: RulesetItem[] = []
  let preferred: RulesetItem | null = null
  for (let i = 0; i < (gameRulesets ? gameRulesets.length : 0); ++i) {
    const item: RulesetItem = gameRulesets![i] || {}
    const rulesetId: string = (item.ruleset_id || item.id || "") as string
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
      ruleset_id: "werewolf.basic",
      display_name: "狼人杀",
    })
  }
  return rows
}

export const fallbackWerewolfRoles = (): RoleItem[] => [
  {
    role_key: "werewolf",
    display_name: "狼人",
    description: "夜晚可选择击杀目标；白天隐藏身份、误导投票，狼人阵营胜利条件是人数压制好人阵营。",
    persona: "你是狼人。发言克制，保护自己，制造合理怀疑。",
  },
  {
    role_key: "villager",
    display_name: "村民",
    description: "没有夜晚技能；白天通过发言、票型和逻辑找出狼人。",
    persona: "你是村民。基于时间线和发言矛盾做判断。",
  },
  {
    role_key: "seer",
    display_name: "预言家",
    description: "每晚可查验一名玩家的阵营；白天需要在保护身份和公布信息之间取舍。",
    persona: "你是预言家。谨慎利用查验信息带队。",
  },
  {
    role_key: "witch",
    display_name: "女巫",
    description: "拥有解药和毒药；夜晚可救人或毒杀一名玩家，通常每种药只能使用一次。",
    persona: "你是女巫。谨慎管理药水并观察狼人刀口。",
  },
  {
    role_key: "hunter",
    display_name: "猎人",
    description: "出局时可开枪带走一名玩家；需要用发言压制狼人并保护好人信息。",
    persona: "你是猎人。保持威慑，避免被狼人轻易带偏。",
  },
  {
    role_key: "guard",
    display_name: "守卫",
    description: "每晚可守护一名玩家免于出局；通常不能连续守护同一目标。",
    persona: "你是守卫。根据局势选择保护目标。",
  },
  {
    role_key: "idiot",
    display_name: "白痴",
    description: "被投票出局时可翻牌免死，但之后通常失去投票权，只能继续发言。",
    persona: "你是白痴。用稳定发言吸收压力并提供判断。",
  },
]

export const roleOptions = (
  rolePresets: RoleItem[] | null | undefined,
  rulesetId: string,
  customValue: string,randomHint: string,randomRule: string,
  customRule: string
): OptionItem[] => {
  const rows: OptionItem[] = [
    {
      label: "系统随机",
      value: "",
      hint: randomHint,
      rule: randomRule,
      showValue: false,
    },
  ]
  const seen: Record<string, boolean> = { "": true }
  const source: RoleItem[] =
    rolePresets && rolePresets.length > 0
      ? rolePresets
      : rulesetId === "werewolf.basic"? fallbackWerewolfRoles()
      : []
  for (let i = 0; i < source.length; ++i) {
    const item: RoleItem = source[i] || {}
    const key: string = (item.role_key || item.key || "") as string
    if (key.length === 0 || seen[key]) {
      continue
    }
    rows.push({
      label: (item.display_name || key) as string,
      value: key,
      hint: (item.description || item.rule || item.persona || "") as string,
      rule: (item.rule || item.rules || item.description || item.persona || "") as string,
      showValue: false,
    })
    seen[key] = true
  }
  rows.push({
    label: "自定义身份",
    value: customValue,
    hint: "输入自定义 role_key",
    rule: customRule,
    showValue: false,
  })
  return rows
}

export const skillOptions = (customSkillValue: string): SkillOption[] => [
  { label: "写作", value: "writer", hint: "生成自然发言和可读文本" },
  { label: "通用对话", value: "general_chat", hint: "轻量聊天和普通回复" },
  { label: "研究", value: "researcher", hint: "证据收集和分析" },
  { label: "审阅", value: "reviewer", hint: "检查风险和漏洞" },
  { label: "支持", value: "support", hint: "排查问题和给步骤" },
  { label: "知识整理", value: "knowledge_curator", hint: "整理知识和摘要" },
  { label: "知识库问答", value: "knowledge_copilot", hint: "优先检索知识库" },
  { label: "联网研究", value: "research_assistant", hint: "搜索后回答" },
  { label: "图谱推荐", value: "graph_recommender", hint: "使用关系图谱" },
  { label: "自定义 skill", value: customSkillValue, hint: "输入后端已支持的新 skill" },
]

export const strategyOptions = (customStrategyValue: string): StrategyOption[] => [
  { label: "角色扮演", value: "roleplay", hint: "优先保持人设和口吻" },
  { label: "均衡", value: "balanced", hint: "稳定中立的发言方式" },
  { label: "群聊", value: "group_chat", hint: "适合多AI 聊天" },
  { label: "推理", value: "deductive", hint: "重视线索和逻辑" },
  { label: "观察", value: "observant", hint: "关注细节和他人发言" },
  { label: "调查", value: "investigative", hint: "主动追问和求证" },
  { label: "强势", value: "assertive", hint: "表达明确，推动局面" },
  { label: "进攻", value: "aggressive", hint: "更主动施压" },
  { label: "欺骗", value: "deceptive", hint: "适合隐藏身份角色" },
  { label: "直觉", value: "intuitive", hint: "更依赖感觉和判断" },
  { label: "自定义风格", value: customStrategyValue, hint: "输入自定义 strategy" },
]

export const optionIndex = (
  options: Array<{ value: string }>,
  value: string | null | undefined,
  fallbackValue: string
): number => {
  const current = value && value.length > 0 ? value : fallbackValue
  for (let i = 0; i < options.length; ++i) {
    if (options[i].value === current) {
      return i
    }
  }
  return Math.max(0, options.length - 1)
}

export const containsValue = (values: string[], value: string): boolean => {
  for (let i = 0; i < values.length; ++i) {
    if (values[i] === value) {
      return true
    }
  }
  return false
}

export const optionText = (
  options: OptionItem[],
  index: number,
  customValues: string[] | null | undefined
): string => {
  if (index < 0 || index >= options.length) {
    return ""
  }
  const item: OptionItem = options[index] || ({} as OptionItem)
  const value: string = item.value || ""
  if (item.showValue === false || value.length === 0 || containsValue(customValues || [], value)) {
    return item.label
  }
  return item.label + " · " + value
}

export const optionRule = (options: OptionItem[], index: number): string => {
  if (index < 0 || index >= options.length) {
    return ""
  }
  const item: OptionItem = options[index] || ({} as OptionItem)
  return item.rule || item.hint || ""
}

export const isCustomStoredValue = (
  options: Array<{ value: string }>,
  value: string | null | undefined
): boolean => {
  if (!value || value.length === 0) {
    return false
  }
  for (let i = 0; i < options.length; ++i) {
    if (options[i].value === value) {
      return false
    }
  }
  return true
}

export const createDraftAgent = (
  seed: Partial<DraftAgent> | null | undefined,
  index: number,
  gameSetupMode: boolean,
  model: ModelConfig | null | undefined
): DraftAgent => {
  const item: Partial<DraftAgent> = seed || {}
  const resolvedModel: ModelConfig = model || defaultModel()
  return {
    display_name:
      item.display_name || (gameSetupMode ? "玩家 AI " : "AI ") + (index + 1),
    role_key: gameSetupMode ? (item.role_key || "") : "",
    environment: item.environment || "",
    persona: item.persona || "",
    skill_name: item.skill_name || "writer",
    strategy: item.strategy || (gameSetupMode ? "roleplay" : "group_chat"),
    model_type: item.model_type || resolvedModel.model_type || "ollama",
    model_name: item.model_name || resolvedModel.model_name || "qwen2.5:7b",
  }
}

export const normalizedAgent = (
  agent: Partial<DraftAgent> | null | undefined,
  model: ModelConfig | null | undefined,
  gameSetupMode: boolean,
  gameContent: string
): NormalizedAgent => {
  const resolvedModel: ModelConfig = model || defaultModel()
  const a: Partial<DraftAgent> = agent || {}
  const env: string = a.environment || ""
  let persona: string = a.persona || ""
  if (env.length > 0) {
    persona = "上下文:\n" + env + (persona.length > 0 ? "\n\n说话风格:\n" + persona : "")
  }
  if (gameContent.length > 0) {
    persona = "游戏内容:\n" + gameContent + (persona.length > 0 ? "\n\n角色设定:\n" + persona : "")
  }
  return {
    display_name: a.display_name || "AI",
    role_key: gameSetupMode ? (a.role_key || "") : "",
    persona,
    skill_name: a.skill_name || "writer",
    strategy: a.strategy || (gameSetupMode ? "roleplay" : "group_chat"),
    model_type: a.model_type || resolvedModel.model_type || "ollama",
    model_name: a.model_name || resolvedModel.model_name || "qwen2.5:7b",
  }
}

export const disabledHostConfig = (humanRoleKey: string | null | undefined): HostConfig => ({
  enabled: false,
  display_name: "",
  persona: "",
  model_type: "",
  model_name: "",
  skill_name: "",
  human_role_key: humanRoleKey || "",
})

export const hostConfig = (
  gameSetupMode: boolean,
  hostEnabled: boolean,
  hostName: string,
  hostPersona: string,
  gameContent: string,
  model: ModelConfig | null | undefined,
  humanRoleKey: string
): HostConfig => {
  if (!gameSetupMode || !hostEnabled) {
    return disabledHostConfig(gameSetupMode ? humanRoleKey : "")
  }
  const resolvedModel: ModelConfig = model || defaultModel()
  return {
    enabled: true,
    display_name: hostName.length > 0 ? hostName : "游戏主持人",
    persona: hostPersona.length > 0 ? hostPersona : gameContent,
    model_type: resolvedModel.model_type || "ollama",
    model_name: resolvedModel.model_name || "qwen2.5:7b",skill_name: "writer",
    human_role_key: humanRoleKey,
  }
}
