.pragma library
const defaultModel = () => ({
  model_type: "ollama",
  model_name: "qwen2.5:7b"
});
const firstModel = (availableModels) => {
  if (availableModels && availableModels.length > 0) {
    return availableModels[0];
  }
  return defaultModel();
};
const modelLabel = (model) => {
  let m = model;
  if (!m) {
    m = defaultModel();
  }
  return String(m.model_type || "") + ":" + String(m.model_name || "");
};
const formalRulesets = (gameRulesets, testRulesetId) => {
  const rows = [];
  let preferred = null;
  for (let i = 0; i < (gameRulesets ? gameRulesets.length : 0); ++i) {
    const item = gameRulesets[i] || {};
    const rulesetId = item.ruleset_id || item.id || "";
    if (rulesetId !== testRulesetId) {
      if (rulesetId === "werewolf.basic") {
        preferred = item;
      } else {
        rows.push(item);
      }
    }
  }
  if (preferred !== null) {
    rows.unshift(preferred);
  }
  if (rows.length === 0) {
    rows.push({
      ruleset_id: "werewolf.basic",
      display_name: "\u72FC\u4EBA\u6740"
    });
  }
  return rows;
};
const fallbackWerewolfRoles = () => [
  {
    role_key: "werewolf",
    display_name: "\u72FC\u4EBA",
    description: "\u591C\u665A\u53EF\u9009\u62E9\u51FB\u6740\u76EE\u6807\uFF1B\u767D\u5929\u9690\u85CF\u8EAB\u4EFD\u3001\u8BEF\u5BFC\u6295\u7968\uFF0C\u72FC\u4EBA\u9635\u8425\u80DC\u5229\u6761\u4EF6\u662F\u4EBA\u6570\u538B\u5236\u597D\u4EBA\u9635\u8425\u3002",
    persona: "\u4F60\u662F\u72FC\u4EBA\u3002\u53D1\u8A00\u514B\u5236\uFF0C\u4FDD\u62A4\u81EA\u5DF1\uFF0C\u5236\u9020\u5408\u7406\u6000\u7591\u3002"
  },
  {
    role_key: "villager",
    display_name: "\u6751\u6C11",
    description: "\u6CA1\u6709\u591C\u665A\u6280\u80FD\uFF1B\u767D\u5929\u901A\u8FC7\u53D1\u8A00\u3001\u7968\u578B\u548C\u903B\u8F91\u627E\u51FA\u72FC\u4EBA\u3002",
    persona: "\u4F60\u662F\u6751\u6C11\u3002\u57FA\u4E8E\u65F6\u95F4\u7EBF\u548C\u53D1\u8A00\u77DB\u76FE\u505A\u5224\u65AD\u3002"
  },
  {
    role_key: "seer",
    display_name: "\u9884\u8A00\u5BB6",
    description: "\u6BCF\u665A\u53EF\u67E5\u9A8C\u4E00\u540D\u73A9\u5BB6\u7684\u9635\u8425\uFF1B\u767D\u5929\u9700\u8981\u5728\u4FDD\u62A4\u8EAB\u4EFD\u548C\u516C\u5E03\u4FE1\u606F\u4E4B\u95F4\u53D6\u820D\u3002",
    persona: "\u4F60\u662F\u9884\u8A00\u5BB6\u3002\u8C28\u614E\u5229\u7528\u67E5\u9A8C\u4FE1\u606F\u5E26\u961F\u3002"
  },
  {
    role_key: "witch",
    display_name: "\u5973\u5DEB",
    description: "\u62E5\u6709\u89E3\u836F\u548C\u6BD2\u836F\uFF1B\u591C\u665A\u53EF\u6551\u4EBA\u6216\u6BD2\u6740\u4E00\u540D\u73A9\u5BB6\uFF0C\u901A\u5E38\u6BCF\u79CD\u836F\u53EA\u80FD\u4F7F\u7528\u4E00\u6B21\u3002",
    persona: "\u4F60\u662F\u5973\u5DEB\u3002\u8C28\u614E\u7BA1\u7406\u836F\u6C34\u5E76\u89C2\u5BDF\u72FC\u4EBA\u5200\u53E3\u3002"
  },
  {
    role_key: "hunter",
    display_name: "\u730E\u4EBA",
    description: "\u51FA\u5C40\u65F6\u53EF\u5F00\u67AA\u5E26\u8D70\u4E00\u540D\u73A9\u5BB6\uFF1B\u9700\u8981\u7528\u53D1\u8A00\u538B\u5236\u72FC\u4EBA\u5E76\u4FDD\u62A4\u597D\u4EBA\u4FE1\u606F\u3002",
    persona: "\u4F60\u662F\u730E\u4EBA\u3002\u4FDD\u6301\u5A01\u6151\uFF0C\u907F\u514D\u88AB\u72FC\u4EBA\u8F7B\u6613\u5E26\u504F\u3002"
  },
  {
    role_key: "guard",
    display_name: "\u5B88\u536B",
    description: "\u6BCF\u665A\u53EF\u5B88\u62A4\u4E00\u540D\u73A9\u5BB6\u514D\u4E8E\u51FA\u5C40\uFF1B\u901A\u5E38\u4E0D\u80FD\u8FDE\u7EED\u5B88\u62A4\u540C\u4E00\u76EE\u6807\u3002",
    persona: "\u4F60\u662F\u5B88\u536B\u3002\u6839\u636E\u5C40\u52BF\u9009\u62E9\u4FDD\u62A4\u76EE\u6807\u3002"
  },
  {
    role_key: "idiot",
    display_name: "\u767D\u75F4",
    description: "\u88AB\u6295\u7968\u51FA\u5C40\u65F6\u53EF\u7FFB\u724C\u514D\u6B7B\uFF0C\u4F46\u4E4B\u540E\u901A\u5E38\u5931\u53BB\u6295\u7968\u6743\uFF0C\u53EA\u80FD\u7EE7\u7EED\u53D1\u8A00\u3002",
    persona: "\u4F60\u662F\u767D\u75F4\u3002\u7528\u7A33\u5B9A\u53D1\u8A00\u5438\u6536\u538B\u529B\u5E76\u63D0\u4F9B\u5224\u65AD\u3002"
  }
];
const roleOptions = (rolePresets, rulesetId, customValue, randomHint, randomRule, customRule) => {
  const rows = [
    {
      label: "\u7CFB\u7EDF\u968F\u673A",
      value: "",
      hint: randomHint,
      rule: randomRule,
      showValue: false
    }
  ];
  const seen = { "": true };
  const source = rolePresets && rolePresets.length > 0 ? rolePresets : rulesetId === "werewolf.basic" ? fallbackWerewolfRoles() : [];
  for (let i = 0; i < source.length; ++i) {
    const item = source[i] || {};
    const key = item.role_key || item.key || "";
    if (key.length === 0 || seen[key]) {
      continue;
    }
    rows.push({
      label: item.display_name || key,
      value: key,
      hint: item.description || item.rule || item.persona || "",
      rule: item.rule || item.rules || item.description || item.persona || "",
      showValue: false
    });
    seen[key] = true;
  }
  rows.push({
    label: "\u81EA\u5B9A\u4E49\u8EAB\u4EFD",
    value: customValue,
    hint: "\u8F93\u5165\u81EA\u5B9A\u4E49 role_key",
    rule: customRule,
    showValue: false
  });
  return rows;
};
const skillOptions = (customSkillValue) => [
  { label: "\u5199\u4F5C", value: "writer", hint: "\u751F\u6210\u81EA\u7136\u53D1\u8A00\u548C\u53EF\u8BFB\u6587\u672C" },
  { label: "\u901A\u7528\u5BF9\u8BDD", value: "general_chat", hint: "\u8F7B\u91CF\u804A\u5929\u548C\u666E\u901A\u56DE\u590D" },
  { label: "\u7814\u7A76", value: "researcher", hint: "\u8BC1\u636E\u6536\u96C6\u548C\u5206\u6790" },
  { label: "\u5BA1\u9605", value: "reviewer", hint: "\u68C0\u67E5\u98CE\u9669\u548C\u6F0F\u6D1E" },
  { label: "\u652F\u6301", value: "support", hint: "\u6392\u67E5\u95EE\u9898\u548C\u7ED9\u6B65\u9AA4" },
  { label: "\u77E5\u8BC6\u6574\u7406", value: "knowledge_curator", hint: "\u6574\u7406\u77E5\u8BC6\u548C\u6458\u8981" },
  { label: "\u77E5\u8BC6\u5E93\u95EE\u7B54", value: "knowledge_copilot", hint: "\u4F18\u5148\u68C0\u7D22\u77E5\u8BC6\u5E93" },
  { label: "\u8054\u7F51\u7814\u7A76", value: "research_assistant", hint: "\u641C\u7D22\u540E\u56DE\u7B54" },
  { label: "\u56FE\u8C31\u63A8\u8350", value: "graph_recommender", hint: "\u4F7F\u7528\u5173\u7CFB\u56FE\u8C31" },
  { label: "\u81EA\u5B9A\u4E49 skill", value: customSkillValue, hint: "\u8F93\u5165\u540E\u7AEF\u5DF2\u652F\u6301\u7684\u65B0 skill" }
];
const strategyOptions = (customStrategyValue) => [
  { label: "\u89D2\u8272\u626E\u6F14", value: "roleplay", hint: "\u4F18\u5148\u4FDD\u6301\u4EBA\u8BBE\u548C\u53E3\u543B" },
  { label: "\u5747\u8861", value: "balanced", hint: "\u7A33\u5B9A\u4E2D\u7ACB\u7684\u53D1\u8A00\u65B9\u5F0F" },
  { label: "\u7FA4\u804A", value: "group_chat", hint: "\u9002\u5408\u591AAI \u804A\u5929" },
  { label: "\u63A8\u7406", value: "deductive", hint: "\u91CD\u89C6\u7EBF\u7D22\u548C\u903B\u8F91" },
  { label: "\u89C2\u5BDF", value: "observant", hint: "\u5173\u6CE8\u7EC6\u8282\u548C\u4ED6\u4EBA\u53D1\u8A00" },
  { label: "\u8C03\u67E5", value: "investigative", hint: "\u4E3B\u52A8\u8FFD\u95EE\u548C\u6C42\u8BC1" },
  { label: "\u5F3A\u52BF", value: "assertive", hint: "\u8868\u8FBE\u660E\u786E\uFF0C\u63A8\u52A8\u5C40\u9762" },
  { label: "\u8FDB\u653B", value: "aggressive", hint: "\u66F4\u4E3B\u52A8\u65BD\u538B" },
  { label: "\u6B3A\u9A97", value: "deceptive", hint: "\u9002\u5408\u9690\u85CF\u8EAB\u4EFD\u89D2\u8272" },
  { label: "\u76F4\u89C9", value: "intuitive", hint: "\u66F4\u4F9D\u8D56\u611F\u89C9\u548C\u5224\u65AD" },
  { label: "\u81EA\u5B9A\u4E49\u98CE\u683C", value: customStrategyValue, hint: "\u8F93\u5165\u81EA\u5B9A\u4E49 strategy" }
];
const optionIndex = (options, value, fallbackValue) => {
  const current = value && value.length > 0 ? value : fallbackValue;
  for (let i = 0; i < options.length; ++i) {
    if (options[i].value === current) {
      return i;
    }
  }
  return Math.max(0, options.length - 1);
};
const containsValue = (values, value) => {
  for (let i = 0; i < values.length; ++i) {
    if (values[i] === value) {
      return true;
    }
  }
  return false;
};
const optionText = (options, index, customValues) => {
  if (index < 0 || index >= options.length) {
    return "";
  }
  const item = options[index] || {};
  const value = item.value || "";
  if (item.showValue === false || value.length === 0 || containsValue(customValues || [], value)) {
    return item.label;
  }
  return item.label + " \xB7 " + value;
};
const optionRule = (options, index) => {
  if (index < 0 || index >= options.length) {
    return "";
  }
  const item = options[index] || {};
  return item.rule || item.hint || "";
};
const isCustomStoredValue = (options, value) => {
  if (!value || value.length === 0) {
    return false;
  }
  for (let i = 0; i < options.length; ++i) {
    if (options[i].value === value) {
      return false;
    }
  }
  return true;
};
const createDraftAgent = (seed, index, gameSetupMode, model) => {
  const item = seed || {};
  const resolvedModel = model || defaultModel();
  return {
    display_name: item.display_name || (gameSetupMode ? "\u73A9\u5BB6 AI " : "AI ") + (index + 1),
    role_key: gameSetupMode ? item.role_key || "" : "",
    environment: item.environment || "",
    persona: item.persona || "",
    skill_name: item.skill_name || "writer",
    strategy: item.strategy || (gameSetupMode ? "roleplay" : "group_chat"),
    model_type: item.model_type || resolvedModel.model_type || "ollama",
    model_name: item.model_name || resolvedModel.model_name || "qwen2.5:7b"
  };
};
const normalizedAgent = (agent, model, gameSetupMode, gameContent) => {
  const resolvedModel = model || defaultModel();
  const a = agent || {};
  const env = a.environment || "";
  let persona = a.persona || "";
  if (env.length > 0) {
    persona = "\u4E0A\u4E0B\u6587:\n" + env + (persona.length > 0 ? "\n\n\u8BF4\u8BDD\u98CE\u683C:\n" + persona : "");
  }
  if (gameContent.length > 0) {
    persona = "\u6E38\u620F\u5185\u5BB9:\n" + gameContent + (persona.length > 0 ? "\n\n\u89D2\u8272\u8BBE\u5B9A:\n" + persona : "");
  }
  return {
    display_name: a.display_name || "AI",
    role_key: gameSetupMode ? a.role_key || "" : "",
    persona,
    skill_name: a.skill_name || "writer",
    strategy: a.strategy || (gameSetupMode ? "roleplay" : "group_chat"),
    model_type: a.model_type || resolvedModel.model_type || "ollama",
    model_name: a.model_name || resolvedModel.model_name || "qwen2.5:7b"
  };
};
const disabledHostConfig = (humanRoleKey) => ({
  enabled: false,
  display_name: "",
  persona: "",
  model_type: "",
  model_name: "",
  skill_name: "",
  human_role_key: humanRoleKey || ""
});
const hostConfig = (gameSetupMode, hostEnabled, hostName, hostPersona, gameContent, model, humanRoleKey) => {
  if (!gameSetupMode || !hostEnabled) {
    return disabledHostConfig(gameSetupMode ? humanRoleKey : "");
  }
  const resolvedModel = model || defaultModel();
  return {
    enabled: true,
    display_name: hostName.length > 0 ? hostName : "\u6E38\u620F\u4E3B\u6301\u4EBA",
    persona: hostPersona.length > 0 ? hostPersona : gameContent,
    model_type: resolvedModel.model_type || "ollama",
    model_name: resolvedModel.model_name || "qwen2.5:7b",
    skill_name: "writer",
    human_role_key: humanRoleKey
  };
};
