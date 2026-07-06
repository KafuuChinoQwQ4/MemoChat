export const USER_PUBLIC_ID_PATTERN = /^u[1-9][0-9]{8}$/;
export const GROUP_PUBLIC_ID_PATTERN = /^g[1-9][0-9]{8}$/i;
export function normalizePublicUserId(value) {
    const id = value?.trim() ?? "";
    return USER_PUBLIC_ID_PATTERN.test(id) ? id : "";
}
export function normalizeGroupPublicId(value) {
    const id = value?.trim() ?? "";
    return GROUP_PUBLIC_ID_PATTERN.test(id) ? id.toLowerCase() : "";
}
export function publicUserIdText(value) {
    return normalizePublicUserId(value) || "未分配用户ID";
}
export function groupPublicIdText(value) {
    return normalizeGroupPublicId(value) || "未分配群号";
}
export function displayNameWithoutInternalId(name, publicId, internalId, fallback) {
    const trimmed = name?.trim() ?? "";
    const internalIdText = internalId > 0 ? String(internalId) : "";
    if (trimmed &&
        trimmed !== internalIdText &&
        trimmed !== `UID ${internalIdText}` &&
        trimmed !== `UID: ${internalIdText}` &&
        trimmed !== `群聊 ${internalIdText}` &&
        trimmed !== `群组 ${internalIdText}` &&
        trimmed !== `群号 ${internalIdText}` &&
        trimmed !== `群号:${internalIdText}` &&
        trimmed !== `群号: ${internalIdText}` &&
        trimmed !== `群号：${internalIdText}` &&
        trimmed !== `群号： ${internalIdText}`) {
        return trimmed;
    }
    return normalizePublicUserId(publicId) || normalizeGroupPublicId(publicId) || fallback;
}
