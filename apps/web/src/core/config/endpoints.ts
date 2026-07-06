/** All HTTP route constants — single source of truth */
export const ENDPOINTS = {
  // Auth
  getVarifyCode:    "/get_varifycode",
  register:         "/user_register",
  login:            "/user_login",
  resetPwd:         "/reset_pwd",
  getUserInfo:      "/get_user_info",
  updateProfile:    "/user_update_profile",

  // Moments
  momentsList:      "/api/moments/list",
  momentsPublish:   "/api/moments/publish",
  momentsDelete:    "/api/moments/delete",
  momentsLike:      "/api/moments/like",
  momentsComment:   "/api/moments/comment",
  momentsDetail:    "/api/moments/detail",
  momentsCommentList: "/api/moments/comment/list",
  momentsCommentLike: "/api/moments/comment/like",

  // AI / Agent
  aiChatStream:     "/ai/chat/stream",
  aiSessionCreate:  "/ai/session/create",
  aiSessionList:    "/ai/session/list",
  aiSessionDelete:  "/ai/session/delete",
  aiSessionUpdate:  "/ai/session/update",
  aiModelList:      "/ai/model/list",
  aiModelRegister:  "/ai/model/register",
  aiModelDelete:    "/ai/model/delete",
  aiKbUpload:       "/ai/kb/upload",
  aiKbSearch:       "/ai/kb/search",
  aiKbList:         "/ai/kb/list",
  aiKbDelete:       "/ai/kb/delete",
  aiMemoryList:     "/ai/memory/list",
  aiMemoryCreate:   "/ai/memory/create",
  aiMemoryDelete:   "/ai/memory/delete",
  aiTaskList:       "/ai/task/list",
  aiTaskCreate:     "/ai/task/create",
  aiTaskCancel:     "/ai/task/cancel",
  aiTaskResume:     "/ai/task/resume",

  // Media
  mediaUpload:      "/media/upload",

  // R18
  r18Sources:       "/api/r18/sources",
  r18Search:        "/api/r18/search",
  r18SourceEnable:  "/api/r18/source/enable",
  r18SourceDisable: "/api/r18/source/disable",
} as const
