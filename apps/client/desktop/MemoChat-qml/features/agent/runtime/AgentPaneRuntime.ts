interface Room {
  room_id?: string;
  ruleset_id?: string;
  ruleset?: string;
  title?: string;
  status?: string;
}

interface GameState {
  room?: Room;
  participants?: unknown[];
  events?: unknown[];
  available_actions?: unknown[];
  status?: string;
  phase?: string;
}

interface Session {
  session_id: string;
  title?: string;
}

interface CurrentSessionTitleParams {
  gameRoomActive: boolean;
  gameState?: GameState | null;
  multiAiRoomActive: boolean;
  sessions?: Session[];
  currentSessionId: string;
}

interface SessionSummaryParams {
  gameRoomActive: boolean;
  gameBusy: boolean;
  multiAiRoomActive: boolean;
  gameError: string;
  gameStatusText: string;
  currentSessionId: string;
  streaming: boolean;
  loading: boolean;
}

export const roomObject = (gameState: GameState | null | undefined, currentGameRoomId: string): Room => {
  const room: Room = gameState && gameState.room ? gameState.room : {};
  const roomId = room.room_id || "";
  if (currentGameRoomId.length > 0 && roomId.length > 0 && roomId !== currentGameRoomId) {
    return {};
  }
  return room;
};

export const rulesetId = (gameState: GameState | null | undefined, currentGameRoomId: string): string => {
  const room = roomObject(gameState, currentGameRoomId);
  return room.ruleset_id || room.ruleset || "";
};

export const currentSessionTitle = (params: CurrentSessionTitleParams): string => {
  if (params.gameRoomActive) {
    const room: Room = params.gameState && params.gameState.room ? params.gameState.room : {};
    return room.title || (params.multiAiRoomActive ? "多 AI 聊天" : "Game 房间");
  }
  if (!params.sessions || params.currentSessionId.length === 0) {
    return "未选择会话";
  }
  for (let i = 0; i < params.sessions.length; ++i) {
    const session = params.sessions[i];
    if (session.session_id === params.currentSessionId) {
      return session.title && session.title.length > 0 ? session.title : "当前会话";
    }
  }
  return "当前会话";
};

export const sessionSummary = (params: SessionSummaryParams): string => {
  if (params.gameRoomActive) {
    if (params.gameBusy) {
      return params.multiAiRoomActive ? "多 AI 正在回复。" : "Game 正在推进。";
    }
    if (params.gameError.length > 0) {
      return params.gameError;
    }
    if (params.gameStatusText.length > 0) {
      return params.gameStatusText;
    }
    return params.multiAiRoomActive ? "当前是多 AI 聊天房间。" : "当前是 Game 房间，可在时间线里提交行动。";
  }
  if (params.currentSessionId.length === 0) {
    return "从左侧选择或新建会话开始。";
  }
  if (params.streaming) {
    return "AI 正在生成回复。";
  }
  if (params.loading) {
    return "AI 正在处理你的问题。";
  }
  return "当前会话已就绪，可以继续追问。";
};

export const participants = (gameState: GameState | null | undefined): unknown[] =>
  gameState && gameState.participants ? gameState.participants : [];

export const events = (gameState: GameState | null | undefined): unknown[] =>
  gameState && gameState.events ? gameState.events : [];

export const availableActions = (gameState: GameState | null | undefined): unknown[] =>
  gameState && gameState.available_actions ? gameState.available_actions : [];

export const roomStatusValue = (gameState: GameState | null | undefined, currentGameRoomId: string): string => {
  const room = roomObject(gameState, currentGameRoomId);
  return (room.status || gameState?.status || gameState?.phase || "").toString().toLowerCase();
};

export const isGameEnded = (gameState: GameState | null | undefined, currentGameRoomId: string): boolean => {
  const value = roomStatusValue(gameState, currentGameRoomId);
  return value === "ended" || value === "finished" || value === "complete" || value === "completed";
};

export const skillModeHint = (agentSkillMode: string): string => {
  if (agentSkillMode === "knowledge") {
    return "优先检索已上传文档后回答。";
  }
  if (agentSkillMode === "research") {
    return "先联网搜索，再基于观察回答。";
  }
  if (agentSkillMode === "graph") {
    return "调用图谱记忆和关系推荐。";
  }
  if (agentSkillMode === "calculate") {
    return "显式调用计算器工具。";
  }
  return "根据问题自动选择技能和工具。";
};
