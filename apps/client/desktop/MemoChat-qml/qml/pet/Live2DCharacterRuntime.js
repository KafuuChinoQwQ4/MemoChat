.pragma library

function pathDirectory(path) {
    path = String(path || "")
    const slash = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"))
    return slash >= 0 ? path.slice(0, slash) : ""
}

function pathFileName(path) {
    path = String(path || "")
    const slash = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"))
    return slash >= 0 ? path.slice(slash + 1) : path
}

function optionalFileLabel(label, value) {
    return label + " " + (String(value || "").length > 0 ? "已识别" : "未声明")
}

function actionKindLabel(kind) {
    if (kind === "motion")
        return "动作"
    if (kind === "expression")
        return "表情"
    return "控制"
}

function assetStatusLabel(status) {
    if (status === "ready")
        return "资源可用"
    if (status === "missing")
        return "资源缺失"
    if (status === "invalid")
        return "格式异常"
    return "待补充资源"
}

function assetStatusTone(status) {
    if (status === "ready")
        return "green"
    if (status === "empty")
        return "blue"
    return "rose"
}

function defaultVoicePath(voiceDirectory, defaultVoice) {
    voiceDirectory = String(voiceDirectory || "")
    defaultVoice = String(defaultVoice || "")
    if (voiceDirectory.length === 0 || defaultVoice.length === 0)
        return ""
    const separator = voiceDirectory.endsWith("/") || voiceDirectory.endsWith("\\") ? "" : "/"
    return voiceDirectory + separator + defaultVoice
}

function languageCode(languageText) {
    if (languageText === "日语")
        return "ja-JP"
    if (languageText === "英语")
        return "en-US"
    if (languageText === "韩语")
        return "ko-KR"
    if (languageText === "法语")
        return "fr-FR"
    if (languageText === "西班牙语")
        return "es-ES"
    return "zh-CN"
}

function voiceTrainingStatusLabel(status, stage) {
    if (status === "submitting")
        return "正在提交"
    if (status === "queued")
        return "训练队列已创建"
    if (status === "ready" || stage === "ready_for_gpt_sovits" || stage === "needs_reference_clip")
        return "GPT-SoVITS 可用"
    if (status === "prepared")
        return "训练准备包已生成"
    if (status === "blocked")
        return "等待授权"
    return "未开始"
}

function normalizedVoiceTrainingState(state) {
    const next = {
        status: String(state.status || ""),
        stage: String(state.stage || ""),
        progress: Number(state.progress || 0),
        message: String(state.message || "")
    }
    if (next.stage !== "ready_for_worker")
        return next

    next.stage = "ready_for_gpt_sovits"
    if (next.status === "prepared" || next.status === "idle" || next.status === "blocked")
        next.status = "ready"
    if (next.progress < 70)
        next.progress = 70
    if (next.message.length === 0 || next.message.toLowerCase().indexOf("worker") >= 0)
        next.message = "声音参考已就绪，可直接用于 GPT-SoVITS 零样本合成。"
    return next
}

function promptPreview(values) {
    return "角色名：" + String(values.characterName || "")
            + "\n定位：" + String(values.roleIdentity || "")
            + "\n关系：" + String(values.relationshipStyle || "")
            + "\n性格标签：" + String(values.personalityTags || "")
            + "\n世界观：" + String(values.worldSetting || "")
            + "\n待机动作：" + String(values.idleMotion || "")
            + "\n说话动作：" + String(values.speakingMotion || "")
            + "\n兜底表情：" + String(values.fallbackExpression || "")
            + "\n说话规则：" + String(values.speechRules || "")
            + "\n常用口头禅：" + String(values.catchphrases || "")
            + "\n禁忌：" + String(values.forbiddenRules || "")
}

function applySettingsToDraft(target, settings, toneCombo, responseLengthCombo, languageCombo) {
    target.characterName = settings.characterName
    target.roleIdentity = settings.roleIdentity
    target.modelRoot = settings.modelRoot
    target.modelJson = settings.modelJson
    target.motionDirectory = settings.motionDirectory
    target.expressionDirectory = settings.expressionDirectory
    target.voiceDirectory = settings.voiceDirectory
    target.defaultVoice = settings.defaultVoice
    target.idleMotion = settings.idleMotion
    target.speakingMotion = settings.speakingMotion
    target.fallbackExpression = settings.fallbackExpression
    target.personalityTags = settings.personalityTags
    target.relationshipStyle = settings.relationshipStyle
    target.worldSetting = settings.worldSetting
    target.speechRules = settings.speechRules
    target.catchphrases = settings.catchphrases
    target.forbiddenRules = settings.forbiddenRules
    target.emotionLevel = settings.emotionLevel
    target.creativityLevel = settings.creativityLevel
    target.voiceSpeed = settings.voiceSpeed
    target.lipSyncSensitivity = settings.lipSyncSensitivity
    target.voiceLipSyncEnabled = settings.voiceLipSyncEnabled
    target.emotionSoundEnabled = settings.emotionSoundEnabled
    target.idleMotionEnabled = settings.idleMotionEnabled
    target.gazeFollowEnabled = settings.gazeFollowEnabled
    target.memoryEnabled = settings.memoryEnabled
    target.interruptEnabled = settings.interruptEnabled
    target.cameraEnabled = settings.cameraEnabled
    target.cloudVisionEnabled = settings.cloudVisionEnabled
    target.autoStartPetOnClientStart = settings.autoStartPetOnClientStart
    target.voiceTrainingConsent = settings.voiceTrainingConsent
    target.voiceTrainingConsentScope = settings.voiceTrainingConsentScope
    target.voiceTrainingJobId = settings.voiceTrainingJobId
    target.voiceTrainingStatus = settings.voiceTrainingStatus
    target.voiceTrainingStage = settings.voiceTrainingStage
    target.voiceTrainingProgress = settings.voiceTrainingProgress
    target.voiceTrainingArtifactPath = settings.voiceTrainingArtifactPath
    target.voiceTrainingMessage = settings.voiceTrainingMessage
    toneCombo.currentIndex = settings.toneIndex
    responseLengthCombo.currentIndex = settings.responseLengthIndex
    languageCombo.currentIndex = settings.languageIndex
    target.draftStatus = settings.statusText
}

function storeDraftToSettings(settings, source, toneIndex, responseLengthIndex, languageIndex) {
    settings.characterName = source.characterName
    settings.roleIdentity = source.roleIdentity
    settings.modelRoot = source.modelRoot
    settings.modelJson = source.modelJson
    settings.motionDirectory = source.motionDirectory
    settings.expressionDirectory = source.expressionDirectory
    settings.voiceDirectory = source.voiceDirectory
    settings.defaultVoice = source.defaultVoice
    settings.idleMotion = source.idleMotion
    settings.speakingMotion = source.speakingMotion
    settings.fallbackExpression = source.fallbackExpression
    settings.personalityTags = source.personalityTags
    settings.relationshipStyle = source.relationshipStyle
    settings.worldSetting = source.worldSetting
    settings.speechRules = source.speechRules
    settings.catchphrases = source.catchphrases
    settings.forbiddenRules = source.forbiddenRules
    settings.emotionLevel = source.emotionLevel
    settings.creativityLevel = source.creativityLevel
    settings.voiceSpeed = source.voiceSpeed
    settings.lipSyncSensitivity = source.lipSyncSensitivity
    settings.voiceLipSyncEnabled = source.voiceLipSyncEnabled
    settings.emotionSoundEnabled = source.emotionSoundEnabled
    settings.idleMotionEnabled = source.idleMotionEnabled
    settings.gazeFollowEnabled = source.gazeFollowEnabled
    settings.memoryEnabled = source.memoryEnabled
    settings.interruptEnabled = source.interruptEnabled
    settings.cameraEnabled = source.cameraEnabled
    settings.cloudVisionEnabled = source.cloudVisionEnabled
    settings.autoStartPetOnClientStart = source.autoStartPetOnClientStart
    settings.voiceTrainingConsent = source.voiceTrainingConsent
    settings.voiceTrainingConsentScope = source.voiceTrainingConsentScope
    settings.voiceTrainingJobId = source.voiceTrainingJobId
    settings.voiceTrainingStatus = source.voiceTrainingStatus
    settings.voiceTrainingStage = source.voiceTrainingStage
    settings.voiceTrainingProgress = source.voiceTrainingProgress
    settings.voiceTrainingArtifactPath = source.voiceTrainingArtifactPath
    settings.voiceTrainingMessage = source.voiceTrainingMessage
    settings.toneIndex = toneIndex
    settings.responseLengthIndex = responseLengthIndex
    settings.languageIndex = languageIndex
}
