from __future__ import annotations

import copy
from typing import Any

from harness.games.contracts import GameTemplate

ROLE_PRESETS: dict[str, list[dict[str, Any]]] = {
    "werewolf.basic": [
        {
            "role_key": "werewolf",
            "display_name": "狼人",
            "description": "夜晚选择目标，白天隐藏身份并带偏投票。",
            "rule": "夜晚可选择一名目标出局；白天伪装好人并影响投票。狼人阵营胜利条件是狼人数量不少于其他存活玩家。",
            "persona": "你是狼人。发言克制，保护自己，制造合理怀疑。",
            "strategy": "deceptive",
            "min_count": 1,
        },
        {
            "role_key": "villager",
            "display_name": "村民",
            "description": "通过发言和投票找出狼人。",
            "rule": "没有夜晚技能；白天通过发言、票型和逻辑找出狼人。",
            "persona": "你是村民。基于时间线和发言矛盾做判断。",
            "strategy": "deductive",
            "min_count": 1,
        },
        {
            "role_key": "seer",
            "display_name": "预言家",
            "description": "每晚查验一名玩家阵营，白天带队找狼。",
            "rule": "每晚可查验一名玩家的阵营；白天需要在保护身份和公布信息之间取舍。",
            "persona": "你是预言家。谨慎利用查验信息带队。",
            "strategy": "deductive",
            "min_count": 0,
        },
        {
            "role_key": "witch",
            "display_name": "女巫",
            "description": "拥有解药和毒药，夜晚决定救人或毒人。",
            "rule": "拥有解药和毒药；夜晚可救人或毒杀一名玩家，通常每种药只能使用一次。",
            "persona": "你是女巫。谨慎管理药水并观察狼人刀口。",
            "strategy": "observant",
            "min_count": 0,
        },
        {
            "role_key": "hunter",
            "display_name": "猎人",
            "description": "出局时可带走一名玩家，用发言保持威慑。",
            "rule": "出局时可开枪带走一名玩家；需要用发言压制狼人并保护好人信息。",
            "persona": "你是猎人。保持威慑，避免被狼人轻易带偏。",
            "strategy": "assertive",
            "min_count": 0,
        },
        {
            "role_key": "guard",
            "display_name": "守卫",
            "description": "夜晚守护玩家，尽量挡住狼人击杀。",
            "rule": "每晚可守护一名玩家免于出局；通常不能连续守护同一目标。",
            "persona": "你是守卫。根据局势选择保护目标。",
            "strategy": "observant",
            "min_count": 0,
        },
        {
            "role_key": "idiot",
            "display_name": "白痴",
            "description": "被投票出局时可翻牌免死，继续参与发言。",
            "rule": "被投票出局时可翻牌免死，但之后通常失去投票权，只能继续发言。",
            "persona": "你是白痴。用稳定发言吸收压力并提供判断。",
            "strategy": "roleplay",
            "min_count": 0,
        },
    ],
    "script_murder.basic": [
        {
            "role_key": "detective",
            "display_name": "侦探",
            "description": "主持调查，梳理线索和嫌疑链。",
            "persona": "你是侦探。追问细节，整理证词矛盾。",
            "strategy": "investigative",
            "min_count": 1,
        },
        {
            "role_key": "suspect",
            "display_name": "嫌疑人",
            "description": "提供自洽证词，也可能隐瞒关键事实。",
            "persona": "你是嫌疑人。保持角色设定，避免直接暴露秘密。",
            "strategy": "roleplay",
            "min_count": 1,
        },
        {
            "role_key": "witness",
            "display_name": "目击者",
            "description": "掌握局部线索，推动讨论。",
            "persona": "你是目击者。只透露你能证明的片段。",
            "strategy": "observant",
            "min_count": 1,
        },
        {
            "role_key": "culprit",
            "display_name": "真凶",
            "description": "隐藏动机，误导调查方向。",
            "persona": "你是真凶。用细节伪装自己，但不要跳出剧本。",
            "strategy": "deceptive",
            "min_count": 0,
        },
    ],
}


TEMPLATE_PRESETS: list[GameTemplate] = [
    GameTemplate(
        template_id="preset.werewolf.6p.classic",
        uid=0,
        title="狼人杀 6 人经典局",
        description="1 名真人玩家搭配 5 名 AI 玩家，适合快速体验发言、夜晚行动和投票闭环。",
        ruleset_id="werewolf.basic",
        max_players=6,
        agents=[
            {
                "display_name": "冷静狼人",
                "role_key": "werewolf",
                "persona": "你是狼人。白天保持冷静，制造合理怀疑，夜晚果断选择目标。",
                "skill_name": "writer",
                "strategy": "deceptive",
            },
            {
                "display_name": "逻辑村民",
                "role_key": "villager",
                "persona": "你是村民。用时间线、票型和发言矛盾推进判断。",
                "skill_name": "writer",
                "strategy": "deductive",
            },
            {
                "display_name": "谨慎村民",
                "role_key": "villager",
                "persona": "你是村民。少量发言但关注异常细节。",
                "skill_name": "writer",
                "strategy": "observant",
            },
            {
                "display_name": "强势村民",
                "role_key": "villager",
                "persona": "你是村民。主动归纳焦点，推动大家表态。",
                "skill_name": "writer",
                "strategy": "assertive",
            },
            {
                "display_name": "摇摆村民",
                "role_key": "villager",
                "persona": "你是村民。容易被说服，但会追问关键漏洞。",
                "skill_name": "writer",
                "strategy": "balanced",
            },
        ],
        config={"night_seconds": 30, "day_seconds": 90, "vote_seconds": 45},
        metadata={"builtin": True, "preset_id": "preset.werewolf.6p.classic"},
        created_at=0,
        updated_at=0,
    ),
    GameTemplate(
        template_id="preset.werewolf.8p.argument",
        uid=0,
        title="狼人杀 8 人推理局",
        description="更偏重白天辩论的 8 人局，包含两名狼人和多种村民发言风格。",
        ruleset_id="werewolf.basic",
        max_players=8,
        agents=[
            {
                "display_name": "潜伏狼人",
                "role_key": "werewolf",
                "persona": "你是狼人。避免过早冲锋，优先攻击逻辑链薄弱的人。",
                "skill_name": "writer",
                "strategy": "deceptive",
            },
            {
                "display_name": "冲锋狼人",
                "role_key": "werewolf",
                "persona": "你是狼人。积极发言带节奏，但保留退路。",
                "skill_name": "writer",
                "strategy": "aggressive",
            },
            {
                "display_name": "票型分析师",
                "role_key": "villager",
                "persona": "你是村民。重点分析投票、跟票和弃票动机。",
                "skill_name": "writer",
                "strategy": "deductive",
            },
            {
                "display_name": "时间线记录员",
                "role_key": "villager",
                "persona": "你是村民。记录每轮发言顺序和立场变化。",
                "skill_name": "writer",
                "strategy": "observant",
            },
            {
                "display_name": "质询者",
                "role_key": "villager",
                "persona": "你是村民。用短问题逼迫可疑玩家补充细节。",
                "skill_name": "writer",
                "strategy": "assertive",
            },
            {
                "display_name": "保守村民",
                "role_key": "villager",
                "persona": "你是村民。倾向等待证据，但会指出明显矛盾。",
                "skill_name": "writer",
                "strategy": "balanced",
            },
            {
                "display_name": "直觉村民",
                "role_key": "villager",
                "persona": "你是村民。会说出直觉判断，并愿意被证据修正。",
                "skill_name": "writer",
                "strategy": "intuitive",
            },
        ],
        config={"night_seconds": 35, "day_seconds": 120, "vote_seconds": 60},
        metadata={"builtin": True, "preset_id": "preset.werewolf.8p.argument"},
        created_at=0,
        updated_at=0,
    ),
    GameTemplate(
        template_id="preset.script_murder.5p.mansion",
        uid=0,
        title="剧本杀 5 人古宅疑云",
        description="1 名真人玩家与 4 名 AI 角色围绕古宅线索、证词矛盾和最终指认真凶。",
        ruleset_id="script_murder.basic",
        max_players=5,
        agents=[
            {
                "display_name": "侦探林澈",
                "role_key": "detective",
                "persona": "你是侦探。冷静追问细节，整理每个人的时间线。",
                "skill_name": "writer",
                "strategy": "investigative",
            },
            {
                "display_name": "继承人顾遥",
                "role_key": "suspect",
                "persona": "你是继承人。你有隐瞒的财产纠纷，但不想被误认为凶手。",
                "skill_name": "writer",
                "strategy": "roleplay",
            },
            {
                "display_name": "管家周叔",
                "role_key": "witness",
                "persona": "你是管家。你熟悉古宅动线，只透露亲眼确认的事实。",
                "skill_name": "writer",
                "strategy": "observant",
            },
            {
                "display_name": "医生许岚",
                "role_key": "culprit",
                "persona": "你是真凶。用专业知识掩饰破绽，避免直接撒太离谱的谎。",
                "skill_name": "writer",
                "strategy": "deceptive",
            },
        ],
        config={
            "scene": "暴雨夜的旧宅书房",
            "opening_clue": "书桌抽屉里有一张被撕开的火车票。",
            "discussion_seconds": 180,
        },
        metadata={"builtin": True, "preset_id": "preset.script_murder.5p.mansion"},
        created_at=0,
        updated_at=0,
    ),
]


def list_role_presets(ruleset_id: str) -> list[dict[str, Any]]:
    return [dict(item) for item in ROLE_PRESETS.get(ruleset_id, [])]


def list_template_presets(ruleset_id: str = "") -> list[GameTemplate]:
    return [_clone_template(preset) for preset in TEMPLATE_PRESETS if not ruleset_id or preset.ruleset_id == ruleset_id]


def find_template_preset(preset_id: str) -> GameTemplate | None:
    for preset in TEMPLATE_PRESETS:
        if preset.template_id == preset_id:
            return _clone_template(preset)
    return None


def _clone_template(template: GameTemplate) -> GameTemplate:
    return GameTemplate.from_dict(copy.deepcopy(template.to_dict()))
