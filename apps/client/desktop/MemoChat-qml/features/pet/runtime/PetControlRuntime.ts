// Source file — compiled to PetControlRuntime.js by `npm run build` (esbuild).
// Do NOT include .pragma library here; it is injected by esbuild as a banner.

interface ControllerState {
  phase?: string
  statusText?: string
  error: string
}

interface ModelData {
  model_type?: string
  model_name?: string
}

export function displayStatus(controller: ControllerState | null | undefined): string {
  if (!controller) {
    return "桌宠"
  }
  const phase = controller.phase || ""
  let status = controller.statusText || ""
  if (controller.error.length > 0) {
    status = controller.error
  }
  return status.length > 0 ? status : (phase.length > 0 ? phase : "桌宠")
}

export function modelProviderAvailable(
  providerAvailable: boolean,
  currentModel: string,
  availableModels: unknown[],
  apiProviderStatus: string
): boolean {
  if (providerAvailable) {
    return true
  }
  if (currentModel.length > 0 || availableModels.length > 0) {
    return true
  }
  return apiProviderStatus.indexOf("已接入") >= 0
}

export function cloudVisionRuntimeEnabled(
  cloudVisionEnabled: boolean,
  localOnlyMode: boolean,
  providerAvailable: boolean
): boolean {
  return cloudVisionEnabled && !localOnlyMode && providerAvailable
}

export function cameraDiagnosticText(
  cameraEnabled: boolean,
  cameraCaptureStatus: string
): string {
  if (!cameraEnabled) {
    return "摄像头关闭"
  }
  return cameraCaptureStatus.length > 0 ? cameraCaptureStatus : "等待摄像头状态"
}

export function cloudVisionDiagnosticText(
  localOnlyMode: boolean,
  cloudVisionEnabled: boolean,
  providerAvailable: boolean
): string {
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

export function retentionDiagnosticText(debugRetentionEnabled: boolean): string {
  return debugRetentionEnabled ? "调试保留开启" : "原始帧不保留"
}

export function actionKindLabel(kind: string): string {
  if (kind === "motion") {
    return "动作"
  }
  if (kind === "expression") {
    return "表情"
  }
  return "控制"
}

export function modelFullName(modelData: ModelData | null | undefined): string {
  if (!modelData) {
    return ""
  }
  return (modelData.model_type || "") + ":" + (modelData.model_name || "")
}

export function apiProviderStatusColor(status: string): string {
  return status.indexOf("已接入") >= 0 ? "#4d7f5c" : "#6a7b92"
}
