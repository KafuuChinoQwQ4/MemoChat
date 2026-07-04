.pragma library
const stringValue = (value) => {
  if (value === void 0 || value === null) {
    return "";
  }
  return String(value);
};
const hasText = (value) => {
  return stringValue(value).trim().length > 0;
};
const isOutgoingMessage = (value) => {
  return value === true || value === "true" || value === 1;
};
const messageSenderName = (isOutgoing, selfName, displayName) => {
  return isOutgoing ? selfName : displayName;
};
const newMessage = (params) => {
  const outgoing = isOutgoingMessage(params.outgoing);
  return {
    outgoing,
    turnId: params.turnId || "",
    msgId: (outgoing ? "pet-user-" : "pet-ai-") + Date.now() + "-" + Math.random().toString(16).slice(2),
    msgType: "text",
    content: params.text || "",
    rawContent: params.text || "",
    translationText: params.translation || "",
    fileName: "",
    senderName: messageSenderName(outgoing, params.selfName || "\u6211", params.displayName || "Live2D"),
    senderUid: 0,
    senderIcon: "",
    showAvatar: true,
    showTimeDivider: false,
    timeDividerText: "",
    messageState: params.state || "sent",
    isReply: false,
    replyToMsgId: "",
    replySender: "",
    replyPreview: "",
    audioUrl: params.audioUrl || "",
    audioState: params.audioState || "idle"
  };
};
const assistantEventKey = (event, turnId, fallbackTurnId) => {
  if (turnId.length > 0) {
    return turnId;
  }
  if (fallbackTurnId.length > 0) {
    return fallbackTurnId;
  }
  const eventId = event && event.event_id ? String(event.event_id).trim() : "";
  if (eventId.length > 0) {
    return eventId;
  }
  if (event && event.seq !== void 0 && event.seq !== null) {
    const seq = String(event.seq).trim();
    if (seq.length > 0) {
      return seq;
    }
  }
  return "";
};
const assistantControllerSnapshot = (controller) => {
  if (!controller) {
    return {
      text: "",
      translation: "",
      audioUrl: "",
      audioState: "",
      turnId: "",
      speechFinal: false
    };
  }
  return {
    text: stringValue(controller.speechText),
    translation: stringValue(controller.speechTranslation),
    audioUrl: stringValue(controller.audioUrl),
    audioState: stringValue(controller.audioState),
    turnId: stringValue(controller.turnId).trim(),
    speechFinal: !!controller.speechFinal
  };
};
const assistantEventSnapshot = (event, controllerSnapshot, pendingTurnId, pendingIndex) => {
  if (!event || event.type !== "pet.control") {
    return { valid: false };
  }
  const text = event.text || {};
  const speech = event.speech || {};
  const audio = event.audio || {};
  const delta = stringValue(text.delta || speech.text_delta || event.speech_text);
  const eventTranslation = stringValue(text.translation || event.speech_translation || speech.translation);
  const eventDisplay = stringValue(text.display || event.speech_display_text || event.speech_text || delta);
  const errorText = stringValue(text.error || speech.error || event.error || event.message);
  const phase = stringValue(event.phase).trim();
  let display = hasText(controllerSnapshot.text) ? controllerSnapshot.text : eventDisplay;
  let translation = hasText(controllerSnapshot.translation) ? controllerSnapshot.translation : eventTranslation;
  if (!hasText(display) && phase === "error") {
    display = hasText(errorText) ? errorText : "\u56DE\u590D\u5F02\u5E38";
  }
  const turnId = stringValue(event.turn_id).trim();
  const eventKey = assistantEventKey(event, turnId, pendingTurnId);
  const audioReady = phase === "audio_ready";
  const isFinal = !!text.final || controllerSnapshot.speechFinal;
  const eventAudioUrl = stringValue(audio.url);
  const eventAudioState = stringValue(audio.state || "idle");
  const audioUrl = audioReady ? eventAudioUrl : hasText(controllerSnapshot.audioUrl) ? controllerSnapshot.audioUrl : eventAudioUrl;
  const audioState = audioReady ? eventAudioState : hasText(controllerSnapshot.audioState) ? controllerSnapshot.audioState : eventAudioState;
  const hasContent = hasText(display) || hasText(translation) || phase === "error" || audioReady || hasText(audioUrl) || isFinal && pendingIndex >= 0;
  const audioReadyPayload = audioReady || hasText(eventAudioUrl) || eventAudioState === "ready" || eventAudioState === "playing";
  const resolvedEventKey = audioReadyPayload && controllerSnapshot.turnId.length > 0 ? controllerSnapshot.turnId : eventKey.length > 0 ? eventKey : controllerSnapshot.turnId;
  return {
    valid: true,
    phase,
    display,
    translation,
    audioUrl,
    audioState,
    hasContent,
    isFinal,
    audioReadyPayload,
    resolvedEventKey
  };
};
const assistantFinalStatus = (audioUrl, voiceCallActive) => {
  return hasText(audioUrl) ? "\u8BED\u97F3\u56DE\u590D\u5DF2\u751F\u6210" : voiceCallActive ? "\u8BED\u97F3\u672A\u751F\u6210\uFF0C\u5DF2\u663E\u793A\u6587\u5B57" : "\u6587\u5B57\u56DE\u590D\u5DF2\u751F\u6210";
};
const assistantProgressStatus = (phase) => {
  if (phase === "speaking") {
    return "\u6B63\u5728\u56DE\u590D";
  }
  if (phase === "error") {
    return "\u56DE\u590D\u5F02\u5E38";
  }
  return "";
};
const languageCodeFromIndex = (index) => {
  switch (index) {
    case 1:
      return "ja-JP";
    case 2:
      return "en-US";
    case 3:
      return "ko-KR";
    case 4:
      return "fr-FR";
    case 5:
      return "es-ES";
    default:
      return "zh-CN";
  }
};
const languageCode = (index) => {
  return languageCodeFromIndex(index);
};
const speechRulesText = (value) => {
  return stringValue(value).trim();
};
const languageCodeFromSettings = (settings) => {
  if (!settings) {
    return "zh-CN";
  }
  const index = settings.languageIndex !== void 0 ? settings.languageIndex : 0;
  return languageCodeFromIndex(index);
};
const speechRulesTextFromSettings = (settings) => {
  if (!settings || settings.speechRules === void 0) {
    return "";
  }
  return speechRulesText(settings.speechRules);
};
const petSettingText = (settings, name) => {
  if (!settings || settings[name] === void 0 || settings[name] === null) {
    return "";
  }
  return stringValue(settings[name]).trim();
};
const settingsVoicePath = (voiceDirectory, defaultVoice) => {
  if (voiceDirectory.length === 0 || defaultVoice.length === 0) {
    return "";
  }
  if (defaultVoice.indexOf("/") === 0 || defaultVoice.indexOf("\\") === 0 || defaultVoice.indexOf(":/") > 0 || /^[A-Za-z]:[\\/]/.test(defaultVoice)) {
    return defaultVoice;
  }
  const separator = voiceDirectory.endsWith("/") || voiceDirectory.endsWith("\\") ? "" : "/";
  return voiceDirectory + separator + defaultVoice;
};
const settingsVoiceName = (defaultVoice, fallbackName) => {
  if (defaultVoice.length > 0) {
    const normalized = defaultVoice.replace(/\\/g, "/");
    const baseName = normalized.split("/").pop();
    return baseName.replace(/\.[^.]+$/, "");
  }
  return fallbackName;
};
const currentModelParts = (rawValue) => {
  const raw = stringValue(rawValue);
  if (raw.length === 0) {
    return { model_type: "", model_name: "" };
  }
  const index = raw.indexOf(":");
  if (index < 0) {
    return { model_type: raw, model_name: "" };
  }
  return {
    model_type: raw.slice(0, index),
    model_name: raw.slice(index + 1)
  };
};
const voiceRuntimeSettings = (settings, fallbackName, language) => {
  const voiceDirectory = petSettingText(settings, "voiceDirectory");
  const defaultVoice = petSettingText(settings, "defaultVoice");
  const voicePath = settingsVoicePath(voiceDirectory, defaultVoice);
  return {
    voiceProvider: voicePath.length > 0 ? "gpt-sovits" : "",
    voiceName: settingsVoiceName(defaultVoice, fallbackName),
    voiceLanguage: language,
    textLanguage: language.split("-")[0].toLowerCase(),
    referenceAudioPath: voicePath,
    voiceDirectory,
    defaultVoice,
    voiceTrainingArtifactPath: petSettingText(settings, "voiceTrainingArtifactPath"),
    referenceAudioSource: voicePath.length > 0 ? "pet_asset_settings" : ""
  };
};
