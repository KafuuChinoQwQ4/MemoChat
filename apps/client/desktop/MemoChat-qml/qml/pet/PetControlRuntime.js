.pragma library

function displayStatus(controller) {
    if (!controller) {
        return "桌宠"
    }
    var phase = controller.phase || ""
    var status = controller.statusText || ""
    if (controller.error.length > 0) {
        status = controller.error
    }
    return status.length > 0 ? status : (phase.length > 0 ? phase : "桌宠")
}

function modelProviderAvailable(providerAvailable, currentModel, availableModels, apiProviderStatus) {
    if (providerAvailable) {
        return true
    }
    if (currentModel.length > 0 || availableModels.length > 0) {
        return true
    }
    return apiProviderStatus.indexOf("已接入") >= 0
}

function cloudVisionRuntimeEnabled(cloudVisionEnabled, localOnlyMode, providerAvailable) {
    return cloudVisionEnabled && !localOnlyMode && providerAvailable
}

function cameraDiagnosticText(cameraEnabled, cameraCaptureStatus) {
    if (!cameraEnabled) {
        return "摄像头关闭"
    }
    return cameraCaptureStatus.length > 0 ? cameraCaptureStatus : "等待摄像头状态"
}

function cloudVisionDiagnosticText(localOnlyMode, cloudVisionEnabled, providerAvailable) {
    if (localOnlyMode) {
        return "云视觉被本地优先锁定"
    }
    if (!cloudVisionEnabled) {
        return "云视觉关闭"
    }
    if (!providerAvailable) {
        return "云视觉等待 AI 提供方"
    }
    return "云视觉已授权"
}

function retentionDiagnosticText(debugRetentionEnabled) {
    return debugRetentionEnabled ? "调试保留开启" : "原始帧不保留"
}

function actionKindLabel(kind) {
    if (kind === "motion") {
        return "动作"
    }
    if (kind === "expression") {
        return "表情"
    }
    return "控制"
}

function modelFullName(modelData) {
    if (!modelData) {
        return ""
    }
    return (modelData.model_type || "") + ":" + (modelData.model_name || "")
}

function apiProviderStatusColor(status) {
    return status.indexOf("已接入") >= 0 ? "#4d7f5c" : "#6a7b92"
}
