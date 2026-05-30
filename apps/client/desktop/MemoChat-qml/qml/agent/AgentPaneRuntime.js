.pragma library

function roomObject(gameState, currentGameRoomId) {
    var room = gameState && gameState.room ? gameState.room : ({})
    var roomId = room.room_id || room.id || ""
    if (currentGameRoomId.length > 0 && roomId.length > 0 && roomId !== currentGameRoomId) {
        return ({})
    }
    return room
}

function rulesetId(gameState, currentGameRoomId) {
    var room = roomObject(gameState, currentGameRoomId)
    return room.ruleset_id || room.ruleset || ""
}

function currentSessionTitle(params) {
    if (params.gameRoomActive) {
        var room = params.gameState && params.gameState.room ? params.gameState.room : ({})
        return room.title || (params.multiAiRoomActive ? "多 AI 聊天" : "Game 房间")
    }
    if (!params.sessions || params.currentSessionId.length === 0) {
        return "未选择会话"
    }
    for (var i = 0; i < params.sessions.length; ++i) {
        var session = params.sessions[i]
        if (session.session_id === params.currentSessionId) {
            return session.title && session.title.length > 0 ? session.title : "当前会话"
        }
    }
    return "当前会话"
}

function sessionSummary(params) {
    if (params.gameRoomActive) {
        if (params.gameBusy) {
            return params.multiAiRoomActive ? "多 AI 正在回复。" : "Game 正在推进。"
        }
        if (params.gameError.length > 0) {
            return params.gameError
        }
        if (params.gameStatusText.length > 0) {
            return params.gameStatusText
        }
        return params.multiAiRoomActive ? "当前是多 AI 聊天房间。" : "当前是 Game 房间，可在时间线里提交行动。"
    }
    if (params.currentSessionId.length === 0) {
        return "从左侧选择或新建会话开始。"
    }
    if (params.streaming) {
        return "AI 正在生成回复。"
    }
    if (params.loading) {
        return "AI 正在处理你的问题。"
    }
    return "当前会话已就绪，可以继续追问。"
}

function participants(gameState) {
    return gameState && gameState.participants ? gameState.participants : []
}

function events(gameState) {
    return gameState && gameState.events ? gameState.events : []
}

function availableActions(gameState) {
    return gameState && gameState.available_actions ? gameState.available_actions : []
}

function roomStatusValue(gameState, currentGameRoomId) {
    var room = roomObject(gameState, currentGameRoomId)
    return (room.status || gameState.status || gameState.phase || "").toString().toLowerCase()
}

function isGameEnded(gameState, currentGameRoomId) {
    var value = roomStatusValue(gameState, currentGameRoomId)
    return value === "ended" || value === "finished" || value === "complete" || value === "completed"
}

function skillModeHint(agentSkillMode) {
    if (agentSkillMode === "knowledge") {
        return "优先检索已上传文档后回答。"
    }
    if (agentSkillMode === "research") {
        return "先联网搜索，再基于观察回答。"
    }
    if (agentSkillMode === "graph") {
        return "调用图谱记忆和关系推荐。"
    }
    if (agentSkillMode === "calculate") {
        return "显式调用计算器工具。"
    }
    return "根据问题自动选择技能和工具。"
}
