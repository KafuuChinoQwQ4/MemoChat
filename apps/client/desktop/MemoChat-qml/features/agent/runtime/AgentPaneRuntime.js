.pragma library
const roomObject = (gameState, currentGameRoomId) => {
  const room = gameState && gameState.room ? gameState.room : {};
  const roomId = room.room_id || "";
  if (currentGameRoomId.length > 0 && roomId.length > 0 && roomId !== currentGameRoomId) {
    return {};
  }
  return room;
};
const rulesetId = (gameState, currentGameRoomId) => {
  const room = roomObject(gameState, currentGameRoomId);
  return room.ruleset_id || room.ruleset || "";
};
const currentSessionTitle = (params) => {
  if (params.gameRoomActive) {
    const room = params.gameState && params.gameState.room ? params.gameState.room : {};
    return room.title || (params.multiAiRoomActive ? "\u591A AI \u804A\u5929" : "Game \u623F\u95F4");
  }
  if (!params.sessions || params.currentSessionId.length === 0) {
    return "\u672A\u9009\u62E9\u4F1A\u8BDD";
  }
  for (let i = 0; i < params.sessions.length; ++i) {
    const session = params.sessions[i];
    if (session.session_id === params.currentSessionId) {
      return session.title && session.title.length > 0 ? session.title : "\u5F53\u524D\u4F1A\u8BDD";
    }
  }
  return "\u5F53\u524D\u4F1A\u8BDD";
};
const sessionSummary = (params) => {
  if (params.gameRoomActive) {
    if (params.gameBusy) {
      return params.multiAiRoomActive ? "\u591A AI \u6B63\u5728\u56DE\u590D\u3002" : "Game \u6B63\u5728\u63A8\u8FDB\u3002";
    }
    if (params.gameError.length > 0) {
      return params.gameError;
    }
    if (params.gameStatusText.length > 0) {
      return params.gameStatusText;
    }
    return params.multiAiRoomActive ? "\u5F53\u524D\u662F\u591A AI \u804A\u5929\u623F\u95F4\u3002" : "\u5F53\u524D\u662F Game \u623F\u95F4\uFF0C\u53EF\u5728\u65F6\u95F4\u7EBF\u91CC\u63D0\u4EA4\u884C\u52A8\u3002";
  }
  if (params.currentSessionId.length === 0) {
    return "\u4ECE\u5DE6\u4FA7\u9009\u62E9\u6216\u65B0\u5EFA\u4F1A\u8BDD\u5F00\u59CB\u3002";
  }
  if (params.streaming) {
    return "AI \u6B63\u5728\u751F\u6210\u56DE\u590D\u3002";
  }
  if (params.loading) {
    return "AI \u6B63\u5728\u5904\u7406\u4F60\u7684\u95EE\u9898\u3002";
  }
  return "\u5F53\u524D\u4F1A\u8BDD\u5DF2\u5C31\u7EEA\uFF0C\u53EF\u4EE5\u7EE7\u7EED\u8FFD\u95EE\u3002";
};
const participants = (gameState) => gameState && gameState.participants ? gameState.participants : [];
const events = (gameState) => gameState && gameState.events ? gameState.events : [];
const availableActions = (gameState) => gameState && gameState.available_actions ? gameState.available_actions : [];
const roomStatusValue = (gameState, currentGameRoomId) => {
  const room = roomObject(gameState, currentGameRoomId);
  return (room.status || (gameState == null ? void 0 : gameState.status) || (gameState == null ? void 0 : gameState.phase) || "").toString().toLowerCase();
};
const isGameEnded = (gameState, currentGameRoomId) => {
  const value = roomStatusValue(gameState, currentGameRoomId);
  return value === "ended" || value === "finished" || value === "complete" || value === "completed";
};
const skillModeHint = (agentSkillMode) => {
  if (agentSkillMode === "knowledge") {
    return "\u4F18\u5148\u68C0\u7D22\u5DF2\u4E0A\u4F20\u6587\u6863\u540E\u56DE\u7B54\u3002";
  }
  if (agentSkillMode === "research") {
    return "\u5148\u8054\u7F51\u641C\u7D22\uFF0C\u518D\u57FA\u4E8E\u89C2\u5BDF\u56DE\u7B54\u3002";
  }
  if (agentSkillMode === "graph") {
    return "\u8C03\u7528\u56FE\u8C31\u8BB0\u5FC6\u548C\u5173\u7CFB\u63A8\u8350\u3002";
  }
  if (agentSkillMode === "calculate") {
    return "\u663E\u5F0F\u8C03\u7528\u8BA1\u7B97\u5668\u5DE5\u5177\u3002";
  }
  return "\u6839\u636E\u95EE\u9898\u81EA\u52A8\u9009\u62E9\u6280\u80FD\u548C\u5DE5\u5177\u3002";
};
