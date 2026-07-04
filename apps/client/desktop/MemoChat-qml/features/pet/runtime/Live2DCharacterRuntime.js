.pragma library
const pathDirectory = (path) => {
  const p = String(path || "");
  const slash = Math.max(p.lastIndexOf("/"), p.lastIndexOf("\\"));
  return slash >= 0 ? p.slice(0, slash) : "";
};
const pathFileName = (path) => {
  const p = String(path || "");
  const slash = Math.max(p.lastIndexOf("/"), p.lastIndexOf("\\"));
  return slash >= 0 ? p.slice(slash + 1) : p;
};
const optionalFileLabel = (label, value) => {
  return label + " " + (String(value || "").length > 0 ? "\u5DF2\u8BC6\u522B" : "\u672A\u58F0\u660E");
};
const actionKindLabel = (kind) => {
  if (kind === "motion")
    return "\u52A8\u4F5C";
  if (kind === "expression")
    return "\u8868\u60C5";
  return "\u63A7\u5236";
};
const assetStatusLabel = (status) => {
  if (status === "ready")
    return "\u8D44\u6E90\u53EF\u7528";
  if (status === "missing")
    return "\u8D44\u6E90\u7F3A\u5931";
  if (status === "invalid")
    return "\u683C\u5F0F\u5F02\u5E38";
  return "\u5F85\u8865\u5145\u8D44\u6E90";
};
const assetStatusTone = (status) => {
  if (status === "ready")
    return "green";
  if (status === "empty")
    return "blue";
  return "rose";
};
const defaultVoicePath = (voiceDirectory, defaultVoice) => {
  const dir = String(voiceDirectory || "");
  const voice = String(defaultVoice || "");
  if (dir.length === 0 || voice.length === 0)
    return "";
  const separator = dir.endsWith("/") || dir.endsWith("\\") ? "" : "/";
  return dir + separator + voice;
};
const languageCode = (languageText) => {
  if (languageText === "\u65E5\u8BED")
    return "ja-JP";
  if (languageText === "\u82F1\u8BED")
    return "en-US";
  if (languageText === "\u97E9\u8BED")
    return "ko-KR";
  if (languageText === "\u6CD5\u8BED")
    return "fr-FR";
  if (languageText === "\u897F\u73ED\u7259\u8BED")
    return "es-ES";
  return "zh-CN";
};
const voiceTrainingStatusLabel = (status, stage) => {
  if (status === "submitting")
    return "\u6B63\u5728\u63D0\u4EA4";
  if (status === "queued")
    return "\u8BAD\u7EC3\u961F\u5217\u5DF2\u521B\u5EFA";
  if (status === "ready" || stage === "ready_for_gpt_sovits" || stage === "needs_reference_clip")
    return "GPT-SoVITS \u53EF\u7528";
  if (status === "prepared")
    return "\u8BAD\u7EC3\u51C6\u5907\u5305\u5DF2\u751F\u6210";
  if (status === "blocked")
    return "\u7B49\u5F85\u6388\u6743";
  return "\u672A\u5F00\u59CB";
};
const normalizedVoiceTrainingState = (state) => {
  const next = {
    status: String(state.status || ""),
    stage: String(state.stage || ""),
    progress: Number(state.progress || 0),
    message: String(state.message || "")
  };
  if (next.stage !== "ready_for_worker")
    return next;
  next.stage = "ready_for_gpt_sovits";
  if (next.status === "prepared" || next.status === "idle" || next.status === "blocked")
    next.status = "ready";
  if (next.progress < 70)
    next.progress = 70;
  if (next.message.length === 0 || next.message.toLowerCase().indexOf("worker") >= 0)
    next.message = "\u58F0\u97F3\u53C2\u8003\u5DF2\u5C31\u7EEA\uFF0C\u53EF\u76F4\u63A5\u7528\u4E8E GPT-SoVITS \u96F6\u6837\u672C\u5408\u6210\u3002";
  return next;
};
const promptPreview = (values) => {
  return "\u89D2\u8272\u540D\uFF1A" + String(values.characterName || "") + "\n\u5B9A\u4F4D\uFF1A" + String(values.roleIdentity || "") + "\n\u5173\u7CFB\uFF1A" + String(values.relationshipStyle || "") + "\n\u6027\u683C\u6807\u7B7E\uFF1A" + String(values.personalityTags || "") + "\n\u4E16\u754C\u89C2\uFF1A" + String(values.worldSetting || "") + "\n\u5F85\u673A\u52A8\u4F5C\uFF1A" + String(values.idleMotion || "") + "\n\u8BF4\u8BDD\u52A8\u4F5C\uFF1A" + String(values.speakingMotion || "") + "\n\u515C\u5E95\u8868\u60C5\uFF1A" + String(values.fallbackExpression || "") + "\n\u8BF4\u8BDD\u89C4\u5219\uFF1A" + String(values.speechRules || "") + "\n\u5E38\u7528\u53E3\u5934\u7985\uFF1A" + String(values.catchphrases || "") + "\n\u7981\u5FCC\uFF1A" + String(values.forbiddenRules || "");
};
const applySettingsToDraft = (target, settings, toneCombo, responseLengthCombo, languageCombo) => {
  target.characterName = settings.characterName;
  target.roleIdentity = settings.roleIdentity;
  target.modelRoot = settings.modelRoot;
  target.modelJson = settings.modelJson;
  target.motionDirectory = settings.motionDirectory;
  target.expressionDirectory = settings.expressionDirectory;
  target.voiceDirectory = settings.voiceDirectory;
  target.defaultVoice = settings.defaultVoice;
  target.idleMotion = settings.idleMotion;
  target.speakingMotion = settings.speakingMotion;
  target.fallbackExpression = settings.fallbackExpression;
  target.personalityTags = settings.personalityTags;
  target.relationshipStyle = settings.relationshipStyle;
  target.worldSetting = settings.worldSetting;
  target.speechRules = settings.speechRules;
  target.catchphrases = settings.catchphrases;
  target.forbiddenRules = settings.forbiddenRules;
  target.emotionLevel = settings.emotionLevel;
  target.creativityLevel = settings.creativityLevel;
  target.voiceSpeed = settings.voiceSpeed;
  target.lipSyncSensitivity = settings.lipSyncSensitivity;
  target.voiceLipSyncEnabled = settings.voiceLipSyncEnabled;
  target.emotionSoundEnabled = settings.emotionSoundEnabled;
  target.idleMotionEnabled = settings.idleMotionEnabled;
  target.gazeFollowEnabled = settings.gazeFollowEnabled;
  target.memoryEnabled = settings.memoryEnabled;
  target.interruptEnabled = settings.interruptEnabled;
  target.cameraEnabled = settings.cameraEnabled;
  target.cloudVisionEnabled = settings.cloudVisionEnabled;
  target.autoStartPetOnClientStart = settings.autoStartPetOnClientStart;
  target.voiceTrainingConsent = settings.voiceTrainingConsent;
  target.voiceTrainingConsentScope = settings.voiceTrainingConsentScope;
  target.voiceTrainingJobId = settings.voiceTrainingJobId;
  target.voiceTrainingStatus = settings.voiceTrainingStatus;
  target.voiceTrainingStage = settings.voiceTrainingStage;
  target.voiceTrainingProgress = settings.voiceTrainingProgress;
  target.voiceTrainingArtifactPath = settings.voiceTrainingArtifactPath;
  target.voiceTrainingMessage = settings.voiceTrainingMessage;
  toneCombo.currentIndex = settings.toneIndex;
  responseLengthCombo.currentIndex = settings.responseLengthIndex;
  languageCombo.currentIndex = settings.languageIndex;
  target.draftStatus = settings.statusText;
};
const storeDraftToSettings = (settings, source, toneIndex, responseLengthIndex, languageIndex) => {
  settings.characterName = source.characterName;
  settings.roleIdentity = source.roleIdentity;
  settings.modelRoot = source.modelRoot;
  settings.modelJson = source.modelJson;
  settings.motionDirectory = source.motionDirectory;
  settings.expressionDirectory = source.expressionDirectory;
  settings.voiceDirectory = source.voiceDirectory;
  settings.defaultVoice = source.defaultVoice;
  settings.idleMotion = source.idleMotion;
  settings.speakingMotion = source.speakingMotion;
  settings.fallbackExpression = source.fallbackExpression;
  settings.personalityTags = source.personalityTags;
  settings.relationshipStyle = source.relationshipStyle;
  settings.worldSetting = source.worldSetting;
  settings.speechRules = source.speechRules;
  settings.catchphrases = source.catchphrases;
  settings.forbiddenRules = source.forbiddenRules;
  settings.emotionLevel = source.emotionLevel;
  settings.creativityLevel = source.creativityLevel;
  settings.voiceSpeed = source.voiceSpeed;
  settings.lipSyncSensitivity = source.lipSyncSensitivity;
  settings.voiceLipSyncEnabled = source.voiceLipSyncEnabled;
  settings.emotionSoundEnabled = source.emotionSoundEnabled;
  settings.idleMotionEnabled = source.idleMotionEnabled;
  settings.gazeFollowEnabled = source.gazeFollowEnabled;
  settings.memoryEnabled = source.memoryEnabled;
  settings.interruptEnabled = source.interruptEnabled;
  settings.cameraEnabled = source.cameraEnabled;
  settings.cloudVisionEnabled = source.cloudVisionEnabled;
  settings.autoStartPetOnClientStart = source.autoStartPetOnClientStart;
  settings.voiceTrainingConsent = source.voiceTrainingConsent;
  settings.voiceTrainingConsentScope = source.voiceTrainingConsentScope;
  settings.voiceTrainingJobId = source.voiceTrainingJobId;
  settings.voiceTrainingStatus = source.voiceTrainingStatus;
  settings.voiceTrainingStage = source.voiceTrainingStage;
  settings.voiceTrainingProgress = source.voiceTrainingProgress;
  settings.voiceTrainingArtifactPath = source.voiceTrainingArtifactPath;
  settings.voiceTrainingMessage = source.voiceTrainingMessage;
  settings.toneIndex = toneIndex;
  settings.responseLengthIndex = responseLengthIndex;
  settings.languageIndex = languageIndex;
};
