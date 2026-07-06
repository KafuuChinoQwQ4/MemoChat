import { jsx as _jsx, jsxs as _jsxs, Fragment as _Fragment } from "react/jsx-runtime";
/** Group feature shell content */
import { useEffect, useMemo, useState } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { useSessionStore } from "@/core/session/sessionStore";
import { ReqId } from "@/core/network/opcodes/reqIds";
import { displayNameWithoutInternalId, groupPublicIdText, normalizePublicUserId, publicUserIdText, } from "@/core/entities/displayIds";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassTextField } from "@/shared/ui/glass/GlassTextField";
const USER_ID_PATTERN = /^u[1-9][0-9]{8}$/;
function roleLabel(role) {
    if (role === "owner")
        return "群主";
    if (role === "admin")
        return "管理员";
    return "成员";
}
function groupDisplayName(group) {
    return displayNameWithoutInternalId(group.name, group.groupCode, group.groupId, "未命名群聊");
}
function friendDisplayName(friend) {
    return displayNameWithoutInternalId(friend.name, friend.userId, friend.uid, "未知用户");
}
export function GroupShellContent() {
    const groupsMap = useEntityStore((s) => s.groups);
    const friendsMap = useEntityStore((s) => s.friends);
    const uid = useSessionStore((s) => s.uid) ?? 0;
    const groups = useMemo(() => Array.from(groupsMap.values()), [groupsMap]);
    const friends = useMemo(() => Array.from(friendsMap.values()), [friendsMap]);
    const [selectedGroupId, setSelectedGroupId] = useState(null);
    const [createOpen, setCreateOpen] = useState(false);
    const [groupName, setGroupName] = useState("");
    const [manualMembers, setManualMembers] = useState("");
    const [selectedUserIds, setSelectedUserIds] = useState(() => new Set());
    const [statusText, setStatusText] = useState("");
    const selectedGroup = groups.find((g) => g.groupId === selectedGroupId) ?? null;
    const gateway = useMemo(() => getGateway(), []);
    useEffect(() => {
        if (groups.length === 0) {
            setSelectedGroupId(null);
            return;
        }
        if (selectedGroupId === null || !groups.some((g) => g.groupId === selectedGroupId)) {
            setSelectedGroupId(groups[0]?.groupId ?? null);
        }
    }, [groups, selectedGroupId]);
    useEffect(() => {
        return gateway.dispatcher.subscribe(ReqId.ID_CREATE_GROUP_RSP, (frame) => {
            try {
                const data = JSON.parse(frame.payload);
                if (data.error === 0) {
                    const groupIdText = groupPublicIdText(data.group_code);
                    setStatusText(data.group_code ? `群聊已创建：${data.name || groupIdText}` : "群聊已创建");
                    setCreateOpen(false);
                    setGroupName("");
                    setManualMembers("");
                    setSelectedUserIds(new Set());
                    if (uid > 0) {
                        gateway.chatTransport.send(ReqId.ID_GET_GROUP_LIST_REQ, JSON.stringify({ fromuid: uid }));
                    }
                    return;
                }
                setStatusText(`创建失败，错误码 ${data.error ?? "unknown"}`);
            }
            catch {
                setStatusText("创建失败，服务响应格式错误");
            }
        });
    }, [gateway, uid]);
    function friendUserId(friendUid) {
        const friend = friendsMap.get(friendUid);
        return publicUserIdText(friend?.userId);
    }
    function toggleUserId(userId) {
        if (!userId)
            return;
        setSelectedUserIds((current) => {
            const next = new Set(current);
            if (next.has(userId)) {
                next.delete(userId);
            }
            else {
                next.add(userId);
            }
            return next;
        });
    }
    function collectMemberUserIds() {
        const ids = new Set();
        for (const id of selectedUserIds) {
            if (id)
                ids.add(id);
        }
        for (const part of manualMembers.split(/[\s,，]+/)) {
            const value = part.trim();
            if (value)
                ids.add(value);
        }
        for (const id of ids) {
            if (!USER_ID_PATTERN.test(id)) {
                setStatusText(`成员ID格式非法：${id}`);
                return null;
            }
        }
        return Array.from(ids);
    }
    function handleCreateGroup() {
        const name = groupName.trim();
        if (uid <= 0) {
            setStatusText("登录状态无效，请重新登录");
            return;
        }
        if (name.length === 0 || name.length > 64) {
            setStatusText("群名称长度需在1-64之间");
            return;
        }
        const memberUserIds = collectMemberUserIds();
        if (memberUserIds === null)
            return;
        gateway.chatTransport.send(ReqId.ID_CREATE_GROUP_REQ, JSON.stringify({
            fromuid: uid,
            name,
            member_limit: 200,
            member_user_ids: memberUserIds,
        }));
        setStatusText("正在创建群聊...");
    }
    return (_jsxs("div", { style: {
            display: "grid",
            gridTemplateColumns: "280px minmax(0, 1fr)",
            height: "100%",
            width: "100%",
            minWidth: 0,
            overflow: "hidden",
        }, children: [_jsxs(GlassScrollArea, { style: { borderRight: "1px solid var(--divider)", padding: "10px 8px" }, children: [_jsxs("div", { style: { padding: "6px 10px 12px", display: "flex", alignItems: "center", justifyContent: "space-between", gap: 8 }, children: [_jsx("div", { style: { fontWeight: 700, fontSize: 16 }, children: "\u7FA4\u7EC4" }), _jsx(GlassButton, { type: "button", onClick: () => setCreateOpen((value) => !value), style: { padding: "5px 9px", fontSize: 12 }, children: "\u5EFA\u7FA4" })] }), createOpen ? (_jsx(GlassSurface, { style: { padding: 10, margin: "0 0 10px", borderRadius: 12 }, children: _jsxs("div", { style: { display: "grid", gap: 8 }, children: [_jsx(GlassTextField, { value: groupName, onChange: (event) => setGroupName(event.target.value), placeholder: "\u7FA4\u540D\u79F0\uFF081-64\uFF09", "aria-label": "\u7FA4\u540D\u79F0" }), _jsx("div", { style: { fontSize: 12, color: "var(--text-secondary)", fontWeight: 700 }, children: "\u9009\u62E9\u597D\u53CB" }), _jsxs("div", { style: { display: "grid", gap: 6, maxHeight: 176, overflowY: "auto", paddingRight: 2 }, children: [friends.map((friend) => {
                                            const userId = normalizePublicUserId(friend.userId);
                                            const name = friendDisplayName(friend);
                                            const checked = selectedUserIds.has(userId);
                                            return (_jsxs("label", { style: {
                                                    display: "flex",
                                                    alignItems: "center",
                                                    gap: 8,
                                                    padding: "7px 8px",
                                                    borderRadius: 9,
                                                    background: checked ? "var(--tint-selected)" : "rgba(255,255,255,0.08)",
                                                    border: "1px solid var(--divider)",
                                                    cursor: userId ? "pointer" : "not-allowed",
                                                }, children: [_jsx("input", { type: "checkbox", checked: checked, disabled: !userId, onChange: () => toggleUserId(userId) }), _jsx(Avatar, { src: friend.icon, name: name, size: 28 }), _jsxs("span", { style: { minWidth: 0 }, children: [_jsx("span", { style: { display: "block", fontSize: 12, fontWeight: 700, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: name }), _jsx("span", { style: { display: "block", fontSize: 11, color: "var(--text-disabled)" }, children: friendUserId(friend.uid) || "未分配用户ID" })] })] }, friend.uid));
                                        }), friends.length === 0 ? (_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", textAlign: "center", padding: 10 }, children: "\u6682\u65E0\u597D\u53CB\uFF0C\u53EF\u624B\u52A8\u586B\u5199\u7528\u6237ID" })) : null] }), _jsx("textarea", { value: manualMembers, onChange: (event) => setManualMembers(event.target.value), placeholder: "\u624B\u52A8\u8865\u5145\u7528\u6237ID\uFF0C\u9017\u53F7\u6216\u7A7A\u683C\u5206\u9694", "aria-label": "\u624B\u52A8\u7FA4\u6210\u5458\u7528\u6237ID", style: {
                                        minHeight: 58,
                                        resize: "vertical",
                                        borderRadius: 10,
                                        border: "1px solid var(--divider)",
                                        background: "rgba(255,255,255,0.10)",
                                        color: "var(--text-primary)",
                                        padding: "8px 10px",
                                        font: "inherit",
                                        fontSize: 13,
                                        outline: "none",
                                    } }), _jsx(GlassButton, { type: "button", variant: "primary", onClick: handleCreateGroup, children: "\u521B\u5EFA\u7FA4\u804A" })] }) })) : null, statusText ? (_jsx("div", { style: { padding: "0 10px 10px", fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.45 }, children: statusText })) : null, groups.map((g) => {
                        const name = groupDisplayName(g);
                        return (_jsxs("button", { onClick: () => setSelectedGroupId(g.groupId), style: {
                                display: "flex", alignItems: "center", gap: 10,
                                padding: "10px 12px", border: "none",
                                background: g.groupId === selectedGroupId ? "var(--tint-selected)" : "transparent",
                                cursor: "pointer", width: "100%", textAlign: "left", borderRadius: 8,
                                color: "var(--text-primary)",
                            }, children: [_jsx(Avatar, { src: g.icon, name: name, size: 38 }), _jsxs("div", { style: { minWidth: 0 }, children: [_jsx("div", { style: { fontWeight: 600, fontSize: 14, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: name }), g.memberCount && (_jsxs("div", { style: { fontSize: 12, color: "var(--text-secondary)" }, children: [g.memberCount, " \u4EBA"] }))] })] }, g.groupId));
                    }), groups.length === 0 && (_jsx("div", { style: { textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }, children: "\u6682\u65E0\u7FA4\u7EC4" }))] }), _jsx("div", { style: { minWidth: 0, padding: 16, overflow: "hidden" }, children: _jsx(GlassSurface, { elevated: true, style: {
                        height: "100%",
                        padding: 28,
                        display: "flex",
                        flexDirection: "column",
                        color: "var(--text-primary)",
                    }, children: selectedGroup ? (_jsxs(_Fragment, { children: [_jsxs("div", { style: { display: "flex", alignItems: "center", gap: 16 }, children: [_jsx(Avatar, { src: selectedGroup.icon, name: groupDisplayName(selectedGroup), size: 64, style: { boxShadow: "0 10px 30px rgba(0,0,0,0.10)" } }), _jsxs("div", { style: { minWidth: 0 }, children: [_jsx("div", { style: { fontSize: 22, fontWeight: 700, lineHeight: 1.2 }, children: groupDisplayName(selectedGroup) }), _jsx("div", { style: { marginTop: 4, fontSize: 13, color: "var(--text-secondary)" }, children: (selectedGroup.memberCount ?? 0) > 0 ? `${selectedGroup.memberCount} 人` : "成员数未知" }), _jsxs("div", { style: { marginTop: 4, fontSize: 12, color: "var(--text-disabled)" }, children: ["\u7FA4\u53F7: ", groupPublicIdText(selectedGroup.groupCode)] })] })] }), _jsx("div", { style: { height: 1, background: "var(--divider)", margin: "24px 0" } }), _jsxs("div", { style: { display: "grid", gap: 12, maxWidth: 560 }, children: [_jsxs("div", { children: [_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }, children: "\u6211\u7684\u8EAB\u4EFD" }), _jsx("div", { style: { fontSize: 14 }, children: roleLabel(selectedGroup.role) })] }), _jsxs("div", { children: [_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }, children: "\u7FA4\u516C\u544A" }), _jsx("div", { style: { fontSize: 14, lineHeight: 1.6, whiteSpace: "pre-wrap", overflowWrap: "anywhere" }, children: selectedGroup.announcement?.trim() || "暂无公告" })] })] })] })) : (_jsx("div", { style: { margin: "auto", color: "var(--text-disabled)", fontSize: 14 }, children: "\u9009\u62E9\u7FA4\u7EC4\u67E5\u770B\u8BE6\u60C5" })) }) })] }));
}
