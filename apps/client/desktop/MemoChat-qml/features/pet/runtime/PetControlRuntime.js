.pragma library
function displayStatus(controller) {
  if (!controller) {
    return "\u684C\u5BA0";
  }
  const phase = controller.phase || "";
  let status = controller.statusText || "";
  if (controller.error.length > 0) {
    status = controller.error;
  }
  return status.length > 0 ? status : phase.length > 0 ? phase : "\u684C\u5BA0";
}
function modelProviderAvailable(providerAvailable, currentModel, availableModels, apiProviderStatus) {
  if (providerAvailable) {
    return true;
  }
  if (currentModel.length > 0 || availableModels.length > 0) {
    return true;
  }
  return apiProviderStatus.indexOf("\u5DF2\u63A5\u5165") >= 0;
}
function cloudVisionRuntimeEnabled(cloudVisionEnabled, localOnlyMode, providerAvailable) {
  return cloudVisionEnabled && !localOnlyMode && providerAvailable;
}
function cameraDiagnosticText(cameraEnabled, cameraCaptureStatus) {
  if (!cameraEnabled) {
    return "\u6444\u50CF\u5934\u5173\u95ED";
  }
  return cameraCaptureStatus.length > 0 ? cameraCaptureStatus : "\u7B49\u5F85\u6444\u50CF\u5934\u72B6\u6001";
}
function cloudVisionDiagnosticText(localOnlyMode, cloudVisionEnabled, providerAvailable) {
  if (localOnlyMode) {
    return "\u4E91\u89C6\u89C9\u88AB\u672C\u5730\u4F18\u5148\u9501\u5B9A";
  }
  if (!cloudVisionEnabled) {
    return "\u4E91\u89C6\u89C9\u5173\u95ED";
  }
  if (!providerAvailable) {
    return "\u4E91\u89C6\u89C9\u7B49\u5F85 AI \u63D0\u4F9B\u65B9";
  }
  return "\u4E91\u89C6\u89C9\u5DF2\u6388\u6743";
}
function retentionDiagnosticText(debugRetentionEnabled) {
  return debugRetentionEnabled ? "\u8C03\u8BD5\u4FDD\u7559\u5F00\u542F" : "\u539F\u59CB\u5E27\u4E0D\u4FDD\u7559";
}
function actionKindLabel(kind) {
  if (kind === "motion") {
    return "\u52A8\u4F5C";
  }
  if (kind === "expression") {
    return "\u8868\u60C5";
  }
  return "\u63A7\u5236";
}
function modelFullName(modelData) {
  if (!modelData) {
    return "";
  }
  return (modelData.model_type || "") + ":" + (modelData.model_name || "");
}
function apiProviderStatusColor(status) {
  return status.indexOf("\u5DF2\u63A5\u5165") >= 0 ? "#4d7f5c" : "#6a7b92";
}
