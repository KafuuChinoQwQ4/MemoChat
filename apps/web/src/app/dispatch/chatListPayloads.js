import { useEntityStore } from "@/core/entities/entityStore";
import { normalizeGroupPublicId } from "@/core/entities/displayIds";
function groupRole(role) {
    if (role === 3)
        return "owner";
    if (role === 2)
        return "admin";
    return "member";
}
export function groupsFromPayload(payload) {
    const data = JSON.parse(payload);
    if (data.error !== undefined && data.error !== 0)
        return [];
    return (data.group_list ?? [])
        .map((row) => {
        const groupId = Number(row.groupid ?? 0);
        if (!Number.isFinite(groupId) || groupId <= 0)
            return null;
        const groupCode = normalizeGroupPublicId(row.group_code);
        return {
            groupId,
            ...(groupCode ? { groupCode } : {}),
            name: row.name ?? (groupCode ? `群组 ${groupCode}` : "未命名群聊"),
            icon: row.icon ?? "",
            memberCount: row.member_count ?? 0,
            announcement: row.announcement ?? "",
            role: groupRole(row.role),
        };
    })
        .filter((group) => group !== null);
}
export function dialogsFromPayload(payload) {
    const data = JSON.parse(payload);
    if (data.error !== undefined && data.error !== 0)
        return [];
    return (data.dialogs ?? [])
        .map((row) => {
        const isGroup = row.dialog_type === "group";
        const peerId = isGroup ? Number(row.group_id ?? 0) : Number(row.peer_uid ?? 0);
        if (!Number.isFinite(peerId) || peerId <= 0)
            return null;
        return {
            ...(row.dialog_id ? { dialogId: row.dialog_id } : {}),
            peerId,
            isGroup,
            ...(row.title ? { title: row.title } : {}),
            ...(row.avatar ? { avatar: row.avatar } : {}),
            lastMsgContent: row.last_msg_preview ?? "",
            lastMsgTime: row.last_msg_ts ?? 0,
            unreadCount: row.unread_count ?? 0,
            isPinned: (row.pinned_rank ?? 0) > 0,
            ...(row.draft_text ? { draftText: row.draft_text } : {}),
        };
    })
        .filter((dialog) => dialog !== null);
}
export function applyGroupListPayload(payload) {
    useEntityStore.getState().setGroups(groupsFromPayload(payload));
}
export function applyDialogListPayload(payload) {
    useEntityStore.getState().setDialogs(dialogsFromPayload(payload));
}
