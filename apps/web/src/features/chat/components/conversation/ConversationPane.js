import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/**
 * ConversationPane — full conversation view: message history + composer.
 * Handles message sending via chatApi (optimistic) and reads from entityStore.
 */
import { useEffect, useRef, useMemo, useState } from "react";
import { useEntityStore } from "@/core/entities/entityStore";
import { useSessionStore } from "@/core/session/sessionStore";
import { displayNameWithoutInternalId, groupPublicIdText } from "@/core/entities/displayIds";
import { useChatStore } from "@/features/chat/store/chatStore";
import { createChatApi } from "@/features/chat/api/chatApi";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { hydrateConversation } from "@/core/entities/persistence/messagePersistence";
import { MessageBubble } from "./MessageBubble";
import { ComposerBar } from "./ComposerBar";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
const AVATAR_STACK_WINDOW_MS = 60000;
const TIME_SEPARATOR_WINDOW_MS = 5 * 60000;
function shouldShowMessageAvatar(previous, current) {
    if (!previous)
        return true;
    if (previous.fromUid !== current.fromUid)
        return true;
    if (previous.timestamp <= 0 || current.timestamp <= 0)
        return true;
    return current.timestamp - previous.timestamp > AVATAR_STACK_WINDOW_MS;
}
function shouldShowTimeSeparator(previous, current) {
    if (!previous)
        return true;
    if (previous.timestamp <= 0 || current.timestamp <= 0)
        return false;
    const previousDate = new Date(previous.timestamp);
    const currentDate = new Date(current.timestamp);
    if (previousDate.toDateString() !== currentDate.toDateString())
        return true;
    return current.timestamp - previous.timestamp > TIME_SEPARATOR_WINDOW_MS;
}
function formatConversationTime(timestamp) {
    if (timestamp <= 0)
        return "";
    const date = new Date(timestamp);
    const now = new Date();
    const time = date.toLocaleTimeString("zh-CN", { hour: "2-digit", minute: "2-digit", hour12: false });
    if (date.toDateString() === now.toDateString())
        return time;
    const yesterday = new Date(now);
    yesterday.setDate(now.getDate() - 1);
    if (date.toDateString() === yesterday.toDateString())
        return `昨天 ${time}`;
    const diffDays = Math.floor((startOfDay(now).getTime() - startOfDay(date).getTime()) / 86400000);
    if (diffDays > 0 && diffDays < 7) {
        const weekday = date.toLocaleDateString("zh-CN", { weekday: "short" });
        return `${weekday} ${time}`;
    }
    const day = date.toLocaleDateString("zh-CN", { month: "numeric", day: "numeric" });
    return `${day} ${time}`;
}
function startOfDay(value) {
    return new Date(value.getFullYear(), value.getMonth(), value.getDate());
}
function smartHistoryContent(messages, myUid) {
    const rows = messages
        .filter((msg) => msg.msgType === "text" && !msg.isRevoked && msg.content.trim())
        .slice(-100)
        .map((msg) => ({
        role: msg.fromUid === myUid ? "me" : "peer",
        from_uid: msg.fromUid,
        content: msg.content,
        timestamp: msg.timestamp,
    }));
    return JSON.stringify(rows);
}
function latestTranslatableMessage(messages, myUid) {
    const textMessages = messages.filter((msg) => msg.msgType === "text" && !msg.isRevoked && msg.content.trim());
    return [...textMessages].reverse().find((msg) => msg.fromUid !== myUid) ?? textMessages[textMessages.length - 1] ?? null;
}
function groupTitleWithoutInternalId(name, publicId, peerId) {
    const trimmed = name?.trim() ?? "";
    if (trimmed && trimmed !== String(peerId) && trimmed !== `群聊 ${peerId}`)
        return trimmed;
    return publicId === "未分配群号" ? "群聊" : `群聊 ${publicId}`;
}
export function ConversationPane({ peerId }) {
    // Subscribe to the messages array for this peer directly — same stable-ref pattern.
    const messagesMap = useEntityStore((s) => s.messages);
    const friendsMap = useEntityStore((s) => s.friends);
    const groupsMap = useEntityStore((s) => s.groups);
    const dialogsMap = useEntityStore((s) => s.dialogs);
    const messages = useMemo(() => messagesMap.get(peerId) ?? [], [messagesMap, peerId]);
    const myUid = useSessionStore((s) => s.uid) ?? 0;
    const { selectedIsGroup } = useChatStore();
    const gateway = useMemo(() => getGateway(), []);
    const api = useMemo(() => createChatApi(gateway.chatTransport, gateway.http), [gateway]);
    const bottomRef = useRef(null);
    const [smartBusy, setSmartBusy] = useState(false);
    const [smartTitle, setSmartTitle] = useState("");
    const [smartResult, setSmartResult] = useState("");
    const [smartError, setSmartError] = useState("");
    const peerTitle = useMemo(() => {
        const dialogTitle = dialogsMap.get(peerId)?.title?.trim();
        if (selectedIsGroup) {
            const group = groupsMap.get(peerId);
            return groupTitleWithoutInternalId(group?.name || dialogTitle, groupPublicIdText(group?.groupCode), peerId);
        }
        const friend = friendsMap.get(peerId);
        return displayNameWithoutInternalId(friend?.name || dialogTitle, friend?.userId, peerId, "未知用户");
    }, [dialogsMap, friendsMap, groupsMap, peerId, selectedIsGroup]);
    useEffect(() => {
        let cancelled = false;
        if (myUid <= 0 || peerId <= 0)
            return () => { cancelled = true; };
        useChatStore.getState().setLoadingHistory(true);
        void hydrateConversation(myUid, peerId)
            .catch((err) => {
            console.warn("[ConversationPane] hydrate failed", err);
        })
            .finally(() => {
            if (cancelled)
                return;
            if (selectedIsGroup) {
                api.fetchGroupHistory(myUid, peerId);
            }
            else {
                api.fetchPrivateHistory(myUid, peerId);
            }
        });
        return () => { cancelled = true; };
    }, [api, myUid, peerId, selectedIsGroup]);
    // Scroll to bottom on new messages
    useEffect(() => {
        bottomRef.current?.scrollIntoView({ behavior: "smooth" });
    }, [messages.length]);
    function handleSend(text) {
        if (myUid <= 0)
            return;
        const clientMsgId = selectedIsGroup
            ? api.sendGroupMessage(myUid, peerId, text)
            : api.sendPrivateMessage(myUid, peerId, text);
        // Optimistic append
        const msg = {
            clientMsgId,
            fromUid: myUid,
            toId: peerId,
            isGroup: selectedIsGroup,
            content: text,
            msgType: "text",
            timestamp: Date.now(),
            state: "sending",
        };
        useEntityStore.getState().appendMessage(peerId, msg);
    }
    async function runSmartAction(kind) {
        if (myUid <= 0) {
            setSmartError("登录状态无效，请重新登录");
            return;
        }
        const titleMap = {
            summary: "会话摘要",
            suggest: "建议回复",
            translate: "消息翻译",
        };
        let content = "";
        const context = {
            dialog: selectedIsGroup ? `group:${peerId}` : `user:${peerId}`,
            title: peerTitle,
        };
        if (kind === "translate") {
            const target = latestTranslatableMessage(messages, myUid);
            if (!target) {
                setSmartError("当前会话没有可翻译的文本消息");
                return;
            }
            content = target.content;
            context["source_lang"] = "自动检测";
            context["message_id"] = target.serverMsgId ?? target.clientMsgId;
        }
        else {
            content = smartHistoryContent(messages, myUid);
            if (content === "[]") {
                setSmartError("当前会话还没有可分析的文本消息");
                return;
            }
            context["max_messages"] = 100;
            if (kind === "suggest") {
                context["format"] = "three_options";
            }
        }
        setSmartBusy(true);
        setSmartTitle(titleMap[kind]);
        setSmartResult("");
        setSmartError("");
        try {
            const response = await api.runSmartFeature(myUid, kind, content, {
                targetLang: "中文",
                context,
            });
            const errorCode = response.error ?? response.code ?? 0;
            if (errorCode !== 0) {
                setSmartError(response.message || `智能请求失败，错误码 ${errorCode}`);
                return;
            }
            const result = (response.result || response.content || "").trim();
            setSmartResult(result || "没有返回内容");
        }
        catch (err) {
            setSmartError(err instanceof Error ? err.message : "智能请求失败");
        }
        finally {
            setSmartBusy(false);
        }
    }
    return (_jsxs("div", { style: {
            height: "100%",
            display: "flex",
            flexDirection: "column",
            background: "linear-gradient(180deg, rgba(246,250,255,0.78), rgba(231,241,252,0.84))",
        }, children: [_jsxs("div", { style: {
                    minHeight: 44,
                    padding: "0 14px",
                    display: "flex",
                    alignItems: "center",
                    gap: 10,
                    borderBottom: "1px solid rgba(148, 163, 184, 0.18)",
                    color: "rgba(15, 23, 42, 0.94)",
                    fontSize: 16,
                    fontWeight: 700,
                    background: "linear-gradient(180deg, rgba(255,255,255,0.68), rgba(255,255,255,0.28))",
                }, children: [_jsx("div", { style: { minWidth: 0, flex: 1, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: peerTitle }), _jsxs("div", { style: { display: "flex", gap: 6, alignItems: "center" }, children: [_jsx(GlassButton, { type: "button", onClick: () => void runSmartAction("summary"), disabled: smartBusy, style: { padding: "5px 9px", fontSize: 12 }, children: "\u6458\u8981" }), _jsx(GlassButton, { type: "button", onClick: () => void runSmartAction("suggest"), disabled: smartBusy, style: { padding: "5px 9px", fontSize: 12 }, children: "\u5EFA\u8BAE" }), _jsx(GlassButton, { type: "button", onClick: () => void runSmartAction("translate"), disabled: smartBusy, style: { padding: "5px 9px", fontSize: 12 }, children: "\u7FFB\u8BD1" })] })] }), (smartResult || smartError || smartBusy) ? (_jsxs(GlassSurface, { style: {
                    margin: "10px 14px 0",
                    padding: "10px 12px",
                    borderRadius: 12,
                    background: smartError ? "rgba(232,65,65,0.08)" : undefined,
                }, children: [_jsxs("div", { style: { display: "flex", alignItems: "center", gap: 8 }, children: [_jsx("div", { style: { flex: 1, minWidth: 0, fontSize: 13, fontWeight: 700 }, children: smartTitle || "智能结果" }), _jsx("button", { type: "button", "aria-label": "\u5173\u95ED\u667A\u80FD\u7ED3\u679C", onClick: () => {
                                    setSmartResult("");
                                    setSmartError("");
                                    setSmartTitle("");
                                }, style: {
                                    border: 0,
                                    background: "transparent",
                                    color: "var(--text-secondary)",
                                    cursor: "pointer",
                                    fontSize: 16,
                                    lineHeight: 1,
                                }, children: "\u00D7" })] }), _jsx("div", { style: {
                            marginTop: 6,
                            maxHeight: 138,
                            overflowY: "auto",
                            whiteSpace: "pre-wrap",
                            overflowWrap: "anywhere",
                            fontSize: 13,
                            lineHeight: 1.55,
                            color: smartError ? "var(--color-badge)" : "var(--text-primary)",
                        }, children: smartBusy ? "处理中..." : smartError || smartResult })] })) : null, _jsxs(GlassScrollArea, { style: { flex: 1, padding: "16px 0 18px" }, children: [messages.map((msg, index) => {
                        const previous = index > 0 ? messages[index - 1] : undefined;
                        const showAvatar = shouldShowMessageAvatar(previous, msg);
                        const timeLabel = shouldShowTimeSeparator(previous, msg) ? formatConversationTime(msg.timestamp) : "";
                        return (_jsxs("div", { children: [timeLabel ? (_jsx("div", { style: {
                                        display: "flex",
                                        justifyContent: "center",
                                        padding: index === 0 ? "10px 0 12px" : "18px 0 12px",
                                    }, children: _jsx("span", { style: {
                                            minHeight: 20,
                                            padding: "2px 10px",
                                            borderRadius: 999,
                                            background: "rgba(158, 174, 193, 0.36)",
                                            color: "rgba(47, 65, 88, 0.72)",
                                            fontSize: 11,
                                            lineHeight: "16px",
                                            fontWeight: 500,
                                        }, children: timeLabel }) })) : null, _jsx(MessageBubble, { message: msg, showAvatar: showAvatar, stacked: !showAvatar })] }, msg.clientMsgId));
                    }), _jsx("div", { ref: bottomRef })] }), _jsx(ComposerBar, { onSend: handleSend })] }));
}
