export const stringValue = (value: unknown): string => {
    if (value === undefined || value === null) {
        return ""
    }
    return String(value)
}

export const hasText = (value: unknown): boolean => {
    return stringValue(value).trim().length > 0
}

export const isOutgoingMessage = (value: unknown): boolean => {
    return value === true || value === "true" || value === 1
}

export const messageSenderName = (isOutgoing: boolean, selfName: string, displayName: string): string => {
    return isOutgoing ? selfName : displayName
}

export interface NewMessageParams {
    outgoing?: unknown
    turnId?: string
    text?: string
    translation?: string
    selfName?: string
    displayName?: string
    state?: string
    audioUrl?: string
    audioState?: string
}

export interface ChatMessage {
    outgoing: boolean
    turnId: string
    msgId: string
    msgType: string
    content: string
    rawContent: string
    translationText: string
    fileName: string
    senderName: string
    senderUid: number
    senderIcon: string
    showAvatar: boolean
    showTimeDivider: boolean
    timeDividerText: string
    messageState: string
    isReply: boolean
    replyToMsgId: string
    replySender: string
    replyPreview: string
    audioUrl: string
    audioState: string
}

export const newMessage = (params: NewMessageParams): ChatMessage => {
    const outgoing = isOutgoingMessage(params.outgoing)
    return {
        outgoing,
        turnId: params.turnId || "",
        msgId: (outgoing ? "pet-user-" : "pet-ai-") + Date.now() + "-" + Math.random().toString(16).slice(2),
        msgType: "text",
        content: params.text || "",
        rawContent: params.text || "",
        translationText: params.translation || "",
        fileName: "",
        senderName: messageSenderName(outgoing, params.selfName || "我", params.displayName || "Live2D"),
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
    }
}

export interface AssistantEvent {
    event_id?: unknown
    seq?: unknown
    type?: string
    phase?: unknown
    turn_id?: unknown
    text?: {
        delta?: unknown
        translation?: unknown
        display?: unknown
        error?: unknown
        final?: unknown
    }
    speech?: {
        text_delta?: unknown
        translation?: unknown
        error?: unknown
    }
    audio?: {
        url?: unknown
        state?: unknown
    }
    speech_text?: unknown
    speech_translation?: unknown
    speech_display_text?: unknown
    error?: unknown
    message?: unknown
}

export const assistantEventKey = (event: AssistantEvent | null | undefined, turnId: string, fallbackTurnId: string): string => {
    if (turnId.length > 0) {
        return turnId
    }
    if (fallbackTurnId.length > 0) {
        return fallbackTurnId
    }
    const eventId = event && event.event_id ? String(event.event_id).trim() : ""
    if (eventId.length > 0) {
        return eventId
    }
    if (event && event.seq !== undefined && event.seq !== null) {
        const seq = String(event.seq).trim()
        if (seq.length > 0) {
            return seq
        }
    }
    return ""
}

export interface ControllerSnapshot {
    text: string
    translation: string
    audioUrl: string
    audioState: string
    turnId: string
    speechFinal: boolean
}

export interface AssistantController {
    speechText?: unknown
    speechTranslation?: unknown
    audioUrl?: unknown
    audioState?: unknown
    turnId?: unknown
    speechFinal?: unknown
}

export const assistantControllerSnapshot = (controller: AssistantController | null | undefined): ControllerSnapshot => {
    if (!controller) {
        return {
            text: "",
            translation: "",
            audioUrl: "",
            audioState: "",
            turnId: "",
            speechFinal: false
        }
    }
    return {
        text: stringValue(controller.speechText),
        translation: stringValue(controller.speechTranslation),
        audioUrl: stringValue(controller.audioUrl),
        audioState: stringValue(controller.audioState),
        turnId: stringValue(controller.turnId).trim(),
        speechFinal:!!controller.speechFinal
    }
}

export interface EventSnapshot {
    valid: boolean
    phase?: string
    display?: string
    translation?: string
    audioUrl?: string
    audioState?: string
    hasContent?: boolean
    isFinal?: boolean
    audioReadyPayload?: boolean
    resolvedEventKey?: string
}

export const assistantEventSnapshot = (
    event: AssistantEvent | null | undefined,
    controllerSnapshot: ControllerSnapshot,
    pendingTurnId: string,
    pendingIndex: number
): EventSnapshot => {
    if (!event || event.type !== "pet.control") {
        return { valid: false }
    }

    const text = event.text || {}
    const speech = event.speech || {}
    const audio = event.audio || {}
    const delta = stringValue(text.delta || speech.text_delta || event.speech_text)
    const eventTranslation = stringValue(text.translation || event.speech_translation || speech.translation)
    const eventDisplay = stringValue(text.display || event.speech_display_text || event.speech_text || delta)
    const errorText = stringValue(text.error || speech.error || event.error || event.message)
    const phase = stringValue(event.phase).trim()
    let display = hasText(controllerSnapshot.text) ? controllerSnapshot.text : eventDisplay
    let translation = hasText(controllerSnapshot.translation) ? controllerSnapshot.translation : eventTranslation
    if (!hasText(display) && phase === "error") {
        display = hasText(errorText) ? errorText : "回复异常"
    }

    const turnId = stringValue(event.turn_id).trim()
    const eventKey = assistantEventKey(event, turnId, pendingTurnId)
    const audioReady = phase === "audio_ready"
    const isFinal = !!text.final || controllerSnapshot.speechFinal
    const eventAudioUrl = stringValue(audio.url)
    const eventAudioState = stringValue(audio.state || "idle")
    const audioUrl = audioReady ? eventAudioUrl : (hasText(controllerSnapshot.audioUrl) ? controllerSnapshot.audioUrl : eventAudioUrl)
    const audioState = audioReady ? eventAudioState : (hasText(controllerSnapshot.audioState) ? controllerSnapshot.audioState : eventAudioState)
    const hasContent = hasText(display)|| hasText(translation)
        || phase === "error"
        || audioReady
        || hasText(audioUrl)
        || (isFinal && pendingIndex >= 0)
    const audioReadyPayload = audioReady || hasText(eventAudioUrl) || eventAudioState === "ready"
        || eventAudioState === "playing"
    const resolvedEventKey = audioReadyPayload && controllerSnapshot.turnId.length > 0
        ? controllerSnapshot.turnId
        : (eventKey.length > 0 ? eventKey : controllerSnapshot.turnId)

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
    }
}

export const assistantFinalStatus = (audioUrl: string, voiceCallActive: boolean): string => {
    return hasText(audioUrl)
        ? "语音回复已生成"
        : (voiceCallActive ? "语音未生成，已显示文字" : "文字回复已生成")
}

export const assistantProgressStatus = (phase: string): string => {
    if (phase === "speaking") {
        return "正在回复"
    }
    if (phase === "error") {
        return "回复异常"
    }
    return ""
}

export const languageCodeFromIndex = (index: number): string => {
    switch (index) {
        case 1:
            return "ja-JP"
        case 2:
            return "en-US"
        case 3:
            return "ko-KR"
        case 4:
            return "fr-FR"
        case 5:
            return "es-ES"
        default:
            return "zh-CN"
    }
}

export const languageCode = (index: number): string => {
    return languageCodeFromIndex(index)
}

export const speechRulesText = (value: unknown): string => {
    return stringValue(value).trim()
}

export interface PetSettings {
    languageIndex?: number
    speechRules?: unknown
    voiceDirectory?: unknown
    defaultVoice?: unknown
    voiceTrainingArtifactPath?: unknown
    [key: string]: unknown
}

export const languageCodeFromSettings = (settings: PetSettings | null | undefined): string => {
    if (!settings) {
        return "zh-CN"
    }
    const index = settings.languageIndex !== undefined ? settings.languageIndex : 0
    return languageCodeFromIndex(index)
}

export const speechRulesTextFromSettings = (settings: PetSettings | null | undefined): string => {
    if (!settings || settings.speechRules === undefined) {
        return ""
    }
    return speechRulesText(settings.speechRules)
}

export const petSettingText = (settings: PetSettings | null | undefined, name: string): string => {
    if (!settings || settings[name] === undefined || settings[name] === null) {
        return ""
    }
    return stringValue(settings[name]).trim()
}

export const settingsVoicePath = (voiceDirectory: string, defaultVoice: string): string => {
    if (voiceDirectory.length === 0 || defaultVoice.length === 0) {
        return ""
    }
    if (defaultVoice.indexOf("/") ===0 || defaultVoice.indexOf("\\") === 0
        || defaultVoice.indexOf(":/") > 0 || /^[A-Za-z]:[\\/]/.test(defaultVoice)) {
        return defaultVoice
    }
    const separator = voiceDirectory.endsWith("/") || voiceDirectory.endsWith("\\") ? "" : "/"
    return voiceDirectory + separator + defaultVoice
}

export const settingsVoiceName = (defaultVoice: string, fallbackName: string): string => {
    if (defaultVoice.length > 0) {
        const normalized = defaultVoice.replace(/\\/g, "/")
        const baseName = normalized.split("/").pop()!
        return baseName.replace(/\.[^.]+$/, "")
    }
    return fallbackName
}

export interface ModelParts {
    model_type: string
    model_name: string
}

export const currentModelParts = (rawValue: unknown): ModelParts => {
    const raw = stringValue(rawValue)
    if (raw.length === 0) {
        return { model_type: "", model_name: "" }
    }
    const index = raw.indexOf(":")
    if (index < 0) {
        return { model_type: raw, model_name: "" }
    }
    return {
        model_type: raw.slice(0, index),
        model_name: raw.slice(index + 1)
    }
}

export interface VoiceRuntimeSettings {
    voiceProvider: string
    voiceName: string
    voiceLanguage: string
    textLanguage: string
    referenceAudioPath: string
    voiceDirectory: string
    defaultVoice: string
    voiceTrainingArtifactPath: string
    referenceAudioSource: string
}

export const voiceRuntimeSettings = (settings: PetSettings | null | undefined, fallbackName: string, language: string): VoiceRuntimeSettings => {
    const voiceDirectory = petSettingText(settings, "voiceDirectory")
    const defaultVoice = petSettingText(settings, "defaultVoice")
    const voicePath = settingsVoicePath(voiceDirectory, defaultVoice)
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
    }
}
