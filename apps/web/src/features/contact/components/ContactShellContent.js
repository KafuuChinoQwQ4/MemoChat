import { jsx as _jsx, jsxs as _jsxs, Fragment as _Fragment } from "react/jsx-runtime";
/** ContactShellContent — friend list + apply list view */
import { useEffect, useMemo, useState } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { useSessionStore } from "@/core/session/sessionStore";
import { ReqId } from "@/core/network/opcodes/reqIds";
import { displayNameWithoutInternalId, publicUserIdText } from "@/core/entities/displayIds";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassTextField } from "@/shared/ui/glass/GlassTextField";
const USER_ID_PATTERN = /^u[1-9][0-9]{8}$/;
function parseSearchUser(payload) {
    try {
        const data = JSON.parse(payload);
        if (data.error !== 0 || !data.uid || data.uid <= 0)
            return null;
        return {
            uid: data.uid,
            name: data.nick?.trim() || data.name?.trim() || data.user_id?.trim() || "未知用户",
            userId: data.user_id?.trim() || "",
            icon: data.icon || "",
            desc: data.desc || "",
        };
    }
    catch {
        return null;
    }
}
export function ContactShellContent() {
    const friendsMap = useEntityStore((s) => s.friends);
    const applyList = useEntityStore((s) => s.applyList);
    const uid = useSessionStore((s) => s.uid) ?? 0;
    const profile = useSessionStore((s) => s.profile);
    const friends = useMemo(() => Array.from(friendsMap.values()), [friendsMap]);
    const pendingApplies = useMemo(() => applyList.filter((item) => item.status === "pending"), [applyList]);
    const [selectedUid, setSelectedUid] = useState(null);
    const [searchUserId, setSearchUserId] = useState("");
    const [remark, setRemark] = useState("");
    const [searchResult, setSearchResult] = useState(null);
    const [statusText, setStatusText] = useState("");
    const [searching, setSearching] = useState(false);
    const selectedFriend = friends.find((f) => f.uid === selectedUid) ?? null;
    const gateway = useMemo(() => getGateway(), []);
    useEffect(() => {
        if (friends.length === 0) {
            setSelectedUid(null);
            return;
        }
        if (selectedUid === null || !friends.some((f) => f.uid === selectedUid)) {
            setSelectedUid(friends[0]?.uid ?? null);
        }
    }, [friends, selectedUid]);
    useEffect(() => {
        const unsubs = [
            gateway.dispatcher.subscribe(ReqId.ID_SEARCH_USER_RSP, (frame) => {
                setSearching(false);
                const result = parseSearchUser(frame.payload);
                setSearchResult(result);
                setStatusText(result ? "已找到用户，可以发送好友申请" : "没有找到这个用户");
            }),
            gateway.dispatcher.subscribe(ReqId.ID_ADD_FRIEND_RSP, (frame) => {
                try {
                    const data = JSON.parse(frame.payload);
                    setStatusText(data.error === 0 ? "好友申请已发送" : `申请失败，错误码 ${data.error ?? "unknown"}`);
                }
                catch {
                    setStatusText("申请失败，服务响应格式错误");
                }
            }),
            gateway.dispatcher.subscribe(ReqId.ID_AUTH_FRIEND_RSP, (frame) => {
                try {
                    const data = JSON.parse(frame.payload);
                    setStatusText(data.error === 0 ? "已同意好友申请" : `同意失败，错误码 ${data.error ?? "unknown"}`);
                }
                catch {
                    setStatusText("同意失败，服务响应格式错误");
                }
            }),
        ];
        return () => unsubs.forEach((unsubscribe) => unsubscribe());
    }, [gateway]);
    function handleSearch() {
        const userId = searchUserId.trim();
        setSearchResult(null);
        if (!USER_ID_PATTERN.test(userId)) {
            setStatusText("请输入有效用户ID，例如 u123456789");
            return;
        }
        setSearching(true);
        setStatusText("正在搜索用户...");
        gateway.chatTransport.send(ReqId.ID_SEARCH_USER_REQ, JSON.stringify({ user_id: userId }));
    }
    function handleAddFriend() {
        if (uid <= 0) {
            setStatusText("登录状态无效，请重新登录");
            return;
        }
        if (!searchResult) {
            setStatusText("请先搜索用户");
            return;
        }
        if (searchResult.uid === uid) {
            setStatusText("不能添加自己");
            return;
        }
        const displayName = profile?.name || profile?.userId || "未知用户";
        gateway.chatTransport.send(ReqId.ID_ADD_FRIEND_REQ, JSON.stringify({
            uid,
            applyname: displayName,
            bakname: remark.trim() || displayName,
            touid: searchResult.uid,
            labels: [],
        }));
        setStatusText("好友申请已提交，等待服务端确认");
    }
    function handleApprove(fromUid, fallbackName) {
        if (uid <= 0 || fromUid <= 0) {
            setStatusText("好友申请参数无效");
            return;
        }
        gateway.chatTransport.send(ReqId.ID_AUTH_FRIEND_REQ, JSON.stringify({
            fromuid: uid,
            touid: fromUid,
            back: fallbackName,
            labels: [],
        }));
        setStatusText("正在同意好友申请...");
    }
    return (_jsxs("div", { style: {
            display: "grid",
            gridTemplateColumns: "280px minmax(0, 1fr)",
            height: "100%",
            width: "100%",
            minWidth: 0,
            overflow: "hidden",
        }, children: [_jsxs(GlassScrollArea, { style: { borderRight: "1px solid var(--divider)", padding: "10px 8px" }, children: [_jsx("div", { style: { padding: "6px 10px 12px", fontWeight: 700, fontSize: 16 }, children: "\u8054\u7CFB\u4EBA" }), _jsxs(GlassSurface, { style: { padding: 10, margin: "0 0 10px", borderRadius: 12 }, children: [_jsx("div", { style: { fontSize: 13, fontWeight: 700, marginBottom: 8 }, children: "\u6DFB\u52A0\u597D\u53CB" }), _jsxs("div", { style: { display: "grid", gap: 8 }, children: [_jsx(GlassTextField, { value: searchUserId, onChange: (event) => setSearchUserId(event.target.value), placeholder: "\u7528\u6237ID\uFF0C\u4F8B\u5982 u123456789", "aria-label": "\u641C\u7D22\u7528\u6237ID" }), _jsx(GlassTextField, { value: remark, onChange: (event) => setRemark(event.target.value), placeholder: "\u9A8C\u8BC1\u4FE1\u606F/\u5907\u6CE8\uFF08\u53EF\u9009\uFF09", "aria-label": "\u597D\u53CB\u7533\u8BF7\u5907\u6CE8" }), _jsx(GlassButton, { type: "button", onClick: handleSearch, disabled: searching, children: searching ? "搜索中" : "搜索" })] }), searchResult ? (_jsxs("div", { style: {
                                    display: "flex",
                                    alignItems: "center",
                                    gap: 9,
                                    marginTop: 10,
                                    padding: 8,
                                    borderRadius: 10,
                                    background: "rgba(255,255,255,0.10)",
                                    border: "1px solid var(--divider)",
                                }, children: [_jsx(Avatar, { src: searchResult.icon, name: searchResult.name, size: 34 }), _jsxs("div", { style: { minWidth: 0, flex: 1 }, children: [_jsx("div", { style: { fontSize: 13, fontWeight: 700, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: searchResult.name }), _jsx("div", { style: { fontSize: 11, color: "var(--text-secondary)" }, children: publicUserIdText(searchResult.userId) })] }), _jsx(GlassButton, { type: "button", onClick: handleAddFriend, style: { padding: "5px 8px", fontSize: 12 }, children: "\u7533\u8BF7" })] })) : null, statusText ? (_jsx("div", { style: { marginTop: 8, fontSize: 12, color: "var(--text-secondary)", lineHeight: 1.45 }, children: statusText })) : null] }), pendingApplies.length > 0 ? (_jsxs(GlassSurface, { style: { padding: 10, margin: "0 0 10px", borderRadius: 12 }, children: [_jsx("div", { style: { fontSize: 13, fontWeight: 700, marginBottom: 8 }, children: "\u597D\u53CB\u7533\u8BF7" }), _jsx("div", { style: { display: "grid", gap: 8 }, children: pendingApplies.map((apply) => {
                                    const name = displayNameWithoutInternalId(apply.nick || apply.name, apply.userId, apply.fromUid, "未知用户");
                                    return (_jsxs("div", { style: {
                                            display: "flex",
                                            gap: 8,
                                            alignItems: "center",
                                            minWidth: 0,
                                        }, children: [_jsx(Avatar, { src: apply.icon, name: name, size: 30 }), _jsxs("div", { style: { minWidth: 0, flex: 1 }, children: [_jsx("div", { style: { fontSize: 12, fontWeight: 700, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: name }), _jsx("div", { style: { fontSize: 11, color: "var(--text-disabled)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: publicUserIdText(apply.userId) })] }), _jsx(GlassButton, { type: "button", onClick: () => handleApprove(apply.fromUid, name), style: { padding: "4px 7px", fontSize: 12 }, children: "\u540C\u610F" })] }, `${apply.fromUid}-${apply.applyTime}`));
                                }) })] })) : null, friends.map((f) => {
                        const name = displayNameWithoutInternalId(f.name, f.userId, f.uid, "未知用户");
                        return (_jsxs("button", { onClick: () => setSelectedUid(f.uid), style: {
                                display: "flex",
                                alignItems: "center",
                                gap: 10,
                                padding: "10px 12px",
                                border: "none",
                                background: f.uid === selectedUid ? "var(--tint-selected)" : "transparent",
                                cursor: "pointer",
                                width: "100%",
                                textAlign: "left",
                                borderRadius: 10,
                                color: "var(--text-primary)",
                                transition: "background 140ms ease, transform 120ms ease",
                            }, children: [_jsx(Avatar, { src: f.icon, name: name, size: 38 }), _jsxs("div", { style: { minWidth: 0 }, children: [_jsx("div", { style: { fontWeight: 600, fontSize: 14, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: name }), _jsx("div", { style: { fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: publicUserIdText(f.userId) })] })] }, f.uid));
                    }), friends.length === 0 && (_jsx("div", { style: { textAlign: "center", color: "var(--text-disabled)", padding: 24, fontSize: 13 }, children: "\u6682\u65E0\u597D\u53CB" }))] }), _jsx("div", { style: { minWidth: 0, padding: 16, overflow: "hidden" }, children: _jsx(GlassSurface, { elevated: true, style: {
                        height: "100%",
                        padding: 28,
                        display: "flex",
                        flexDirection: "column",
                        color: "var(--text-primary)",
                    }, children: selectedFriend ? (_jsxs(_Fragment, { children: [_jsxs("div", { style: { display: "flex", alignItems: "center", gap: 16 }, children: [_jsx(Avatar, { src: selectedFriend.icon, name: displayNameWithoutInternalId(selectedFriend.name, selectedFriend.userId, selectedFriend.uid, "未知用户"), size: 64, style: { boxShadow: "0 10px 30px rgba(0,0,0,0.10)" } }), _jsxs("div", { style: { minWidth: 0 }, children: [_jsx("div", { style: { fontSize: 22, fontWeight: 700, lineHeight: 1.2 }, children: displayNameWithoutInternalId(selectedFriend.name, selectedFriend.userId, selectedFriend.uid, "未知用户") }), _jsx("div", { style: { marginTop: 4, fontSize: 13, color: "var(--text-secondary)" }, children: publicUserIdText(selectedFriend.userId) })] })] }), _jsx("div", { style: { height: 1, background: "var(--divider)", margin: "24px 0" } }), _jsxs("div", { style: { display: "grid", gap: 12, maxWidth: 520 }, children: [_jsxs("div", { children: [_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }, children: "\u5907\u6CE8" }), _jsx("div", { style: { fontSize: 14 }, children: selectedFriend.desc?.trim() || "暂无备注" })] }), _jsxs("div", { children: [_jsx("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginBottom: 4 }, children: "\u8D26\u53F7\u72B6\u6001" }), _jsx("div", { style: { fontSize: 14 }, children: "\u5DF2\u6DFB\u52A0\u4E3A\u597D\u53CB" })] })] })] })) : (_jsx("div", { style: { margin: "auto", color: "var(--text-disabled)", fontSize: 14 }, children: "\u9009\u62E9\u8054\u7CFB\u4EBA\u67E5\u770B\u8BE6\u60C5" })) }) })] }));
}
