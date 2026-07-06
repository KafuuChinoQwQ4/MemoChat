import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/**
 * AgentShellContent — AI chat interface with local conversation list + SSE streaming.
 */
import { useState, useRef, useEffect, useMemo } from "react";
import { useAgentStore } from "@/features/agent/store/agentStore";
import { providerAvatarDataUrl, providerDisplayName, providerPalette, } from "@/features/agent/runtime/agentProviderRuntime";
import { loadAgentConversationState, saveAgentConversationState, } from "@/features/agent/runtime/agentConversationPersistence";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { ENDPOINTS } from "@/core/config/endpoints";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassTextField } from "@/shared/ui/glass/GlassTextField";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { Spinner } from "@/shared/ui/primitives/Spinner";
import { useSessionStore } from "@/core/session/sessionStore";
import { formatMessageTime } from "@/shared/lib/time";
function createConversation() {
    return {
        id: `local-${Date.now()}-${Math.random().toString(36).slice(2, 8)}`,
        title: "新对话",
        updatedAt: Date.now(),
        messages: [],
    };
}
function createConversationState() {
    const conversation = createConversation();
    return {
        conversations: [conversation],
        selectedConversationId: conversation.id,
    };
}
function titleFromMessage(content) {
    const normalized = content.replace(/\s+/g, " ").trim();
    return normalized.length > 18 ? `${normalized.slice(0, 18)}...` : normalized || "新对话";
}
function modelKey(model) {
    return `${model.model_type}:${model.model_name}`;
}
function modelDisplay(model) {
    if (!model)
        return "默认模型";
    return model.display_name?.trim() || model.model_name || modelKey(model);
}
function compactModelName(model) {
    if (!model)
        return "未选择模型";
    const provider = providerDisplayName(model.model_name, model.model_type);
    return `${provider} · ${model.model_name}`;
}
function AgentReadyIcon() {
    return (_jsxs("svg", { width: "32", height: "32", viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: "1.6", strokeLinecap: "round", strokeLinejoin: "round", "aria-hidden": true, children: [_jsx("rect", { x: "6.2", y: "7.2", width: "11.6", height: "9.8", rx: "3" }), _jsx("path", { d: "M9.5 11.2h.1" }), _jsx("path", { d: "M14.4 11.2h.1" }), _jsx("path", { d: "M10 14.1c1.2.8 2.8.8 4 0" }), _jsx("path", { d: "M12 7.2V4.8" }), _jsx("path", { d: "M8.2 19.2h7.6" })] }));
}
function ProviderAvatar({ modelName, modelType, size = 34 }) {
    const palette = providerPalette(modelName, modelType);
    return (_jsx("div", { title: palette.display, style: {
            width: size,
            height: size,
            borderRadius: "50%",
            flexShrink: 0,
            display: "grid",
            placeItems: "center",
            overflow: "hidden",
            background: "rgba(255,255,255,0.36)",
            border: "1px solid rgba(255,255,255,0.54)",
            boxShadow: "0 8px 18px rgba(8,15,30,0.14)",
        }, children: _jsx("img", { src: providerAvatarDataUrl(modelName, modelType), alt: palette.display, draggable: false, style: { width: Math.round(size * 0.78), height: Math.round(size * 0.78), objectFit: "contain" } }) }));
}
function MessageRow({ message, userAvatar, userName, streaming = false, }) {
    const isUser = message.role === "user";
    const bubbleRadius = isUser ? "18px 18px 5px 18px" : "18px 18px 18px 5px";
    const providerName = providerDisplayName(message.modelName, message.modelType);
    return (_jsxs("div", { style: {
            marginBottom: 10,
            display: "flex",
            justifyContent: isUser ? "flex-end" : "flex-start",
            alignItems: "flex-start",
            gap: 8,
            animation: "fade-up 180ms cubic-bezier(0.4,0,0.2,1) both",
        }, children: [!isUser && _jsx(ProviderAvatar, { modelName: message.modelName, modelType: message.modelType }), _jsxs(GlassSurface, { style: {
                    maxWidth: "min(72%, 560px)",
                    minWidth: streaming && message.content.length === 0 ? 88 : undefined,
                    padding: "10px 14px",
                    borderRadius: bubbleRadius,
                    background: isUser ? "var(--color-brand-green)" : undefined,
                    color: isUser ? "#fff" : "var(--text-primary)",
                    fontSize: 14,
                    lineHeight: 1.65,
                    whiteSpace: "pre-wrap",
                    overflowWrap: "anywhere",
                    boxShadow: isUser
                        ? "0 2px 10px rgba(7,193,96,0.30)"
                        : "0 1px 4px rgba(0,0,0,0.07), 0 4px 16px rgba(0,0,0,0.06)",
                }, children: [!isUser && (_jsx("div", { style: { fontSize: 11, fontWeight: 700, color: "var(--text-secondary)", marginBottom: 3 }, children: providerName })), message.content, streaming && (_jsx("span", { style: {
                            display: "inline-block",
                            animation: "mc-spin 0.9s ease-in-out infinite",
                            marginLeft: 1,
                        }, children: "\u258B" }))] }), isUser && (_jsx(Avatar, { src: userAvatar, name: userName || "我", size: 34, style: {
                    border: "1px solid rgba(255,255,255,0.54)",
                    boxShadow: "0 8px 18px rgba(8,15,30,0.14)",
                } }))] }));
}
export function AgentShellContent() {
    const streaming = useAgentStore((s) => s.streaming);
    const error = useAgentStore((s) => s.error);
    const appendStreamChunk = useAgentStore((s) => s.appendStreamChunk);
    const finalizeStream = useAgentStore((s) => s.finalizeStream);
    const setStreaming = useAgentStore((s) => s.setStreaming);
    const setError = useAgentStore((s) => s.setError);
    const uid = useSessionStore((s) => s.uid);
    const token = useSessionStore((s) => s.token);
    const profile = useSessionStore((s) => s.profile);
    const initialConversationState = useRef(createConversationState());
    const [conversations, setConversations] = useState(initialConversationState.current.conversations);
    const [selectedConversationId, setSelectedConversationId] = useState(initialConversationState.current.selectedConversationId);
    const [persistenceOwnerUid, setPersistenceOwnerUid] = useState(null);
    const [input, setInput] = useState("");
    const [currentReply, setCurrentReply] = useState("");
    const [availableModels, setAvailableModels] = useState([]);
    const [selectedModelKey, setSelectedModelKey] = useState("");
    const [modelsLoading, setModelsLoading] = useState(false);
    const bottomRef = useRef(null);
    const replyRef = useRef("");
    const replyFinalizedRef = useRef(false);
    const streamModelRef = useRef(null);
    const activeConversation = conversations.find((item) => item.id === selectedConversationId) ?? conversations[0];
    const activeMessages = activeConversation?.messages ?? [];
    const orderedConversations = [...conversations].sort((a, b) => b.updatedAt - a.updatedAt);
    const selectedModel = useMemo(() => {
        if (availableModels.length === 0)
            return null;
        return availableModels.find((model) => modelKey(model) === selectedModelKey) ?? availableModels[0] ?? null;
    }, [availableModels, selectedModelKey]);
    const currentStreamModel = streamModelRef.current;
    const userName = profile?.name?.trim() || (uid !== null ? `u${uid}` : "我");
    /* Auto-scroll on new content */
    useEffect(() => {
        bottomRef.current?.scrollIntoView({ behavior: "smooth" });
    }, [activeMessages.length, currentReply, selectedConversationId]);
    useEffect(() => {
        const restored = uid !== null ? loadAgentConversationState(uid) : null;
        const nextState = restored ?? createConversationState();
        setConversations(nextState.conversations);
        setSelectedConversationId(nextState.selectedConversationId);
        setInput("");
        setCurrentReply("");
        replyRef.current = "";
        replyFinalizedRef.current = false;
        streamModelRef.current = null;
        setPersistenceOwnerUid(uid);
    }, [uid]);
    useEffect(() => {
        if (uid === null || persistenceOwnerUid !== uid)
            return;
        saveAgentConversationState(uid, {
            conversations,
            selectedConversationId,
        });
    }, [uid, persistenceOwnerUid, conversations, selectedConversationId]);
    useEffect(() => {
        if (uid === null || !token) {
            setAvailableModels([]);
            setSelectedModelKey("");
            return;
        }
        const controller = new AbortController();
        let cancelled = false;
        setModelsLoading(true);
        getGateway().http.get(ENDPOINTS.aiModelList, {
            signal: controller.signal,
            headers: {
                "X-User-Id": String(uid),
                "X-User-Token": token,
            },
        }).then((response) => {
            if (cancelled)
                return;
            const seen = new Set();
            const models = (response.models ?? []).filter((model) => {
                if (!model.model_type || !model.model_name)
                    return false;
                const key = modelKey(model);
                if (seen.has(key))
                    return false;
                seen.add(key);
                return true;
            });
            const defaultKey = response.default_model ? modelKey(response.default_model) : models[0] ? modelKey(models[0]) : "";
            setAvailableModels(models);
            setSelectedModelKey((current) => (current && seen.has(current) ? current : defaultKey));
        }).catch((err) => {
            if (err.name !== "AbortError") {
                setError(err instanceof Error ? err.message : "模型列表加载失败");
            }
        }).finally(() => {
            if (!cancelled)
                setModelsLoading(false);
        });
        return () => {
            cancelled = true;
            controller.abort();
        };
    }, [uid, token, setError]);
    function appendToConversation(id, message) {
        setConversations((prev) => prev.map((item) => {
            if (item.id !== id)
                return item;
            const messages = [...item.messages, message];
            return {
                ...item,
                messages,
                title: item.messages.length === 0 && message.role === "user"
                    ? titleFromMessage(message.content)
                    : item.title,
                updatedAt: Date.now(),
            };
        }));
    }
    function finishAssistantReply() {
        if (replyFinalizedRef.current) {
            finalizeStream();
            return;
        }
        replyFinalizedRef.current = true;
        const reply = replyRef.current;
        const streamModel = streamModelRef.current;
        if (reply && streamModel) {
            appendToConversation(streamModel.conversationId, {
                role: "assistant",
                content: reply,
                modelType: streamModel.modelType,
                modelName: streamModel.modelName,
            });
        }
        replyRef.current = "";
        streamModelRef.current = null;
        setCurrentReply("");
        finalizeStream();
    }
    function startNewConversation() {
        const next = createConversation();
        setConversations((prev) => [next, ...prev]);
        setSelectedConversationId(next.id);
        setInput("");
        setCurrentReply("");
        replyRef.current = "";
        replyFinalizedRef.current = false;
        streamModelRef.current = null;
        setError(null);
    }
    async function sendMessage() {
        if (!input.trim() || streaming || !activeConversation)
            return;
        if (uid === null || !token) {
            setError("登录状态已失效，请重新登录");
            return;
        }
        const modelSnapshot = {
            modelType: selectedModel?.model_type ?? "",
            modelName: selectedModel?.model_name ?? "",
        };
        const userMsg = {
            role: "user",
            content: input.trim(),
            modelType: modelSnapshot.modelType,
            modelName: modelSnapshot.modelName,
        };
        const conversationId = activeConversation.id;
        const requestMessages = [...activeMessages, userMsg];
        appendToConversation(conversationId, userMsg);
        setInput("");
        setCurrentReply("");
        replyRef.current = "";
        replyFinalizedRef.current = false;
        streamModelRef.current = { conversationId, ...modelSnapshot };
        setError(null);
        setStreaming(true);
        try {
            const sse = getGateway().sse;
            await sse.start(ENDPOINTS.aiChatStream, {
                uid,
                content: userMsg.content,
                stream: true,
                model_type: modelSnapshot.modelType,
                model_name: modelSnapshot.modelName,
                metadata: {
                    messages: requestMessages.map((m) => ({ role: m.role, content: m.content })),
                },
            }, {
                uid,
                token: token ?? undefined,
                onChunk: (chunk) => {
                    if (chunk.chunk) {
                        const next = replyRef.current + chunk.chunk;
                        replyRef.current = next;
                        setCurrentReply(next);
                        appendStreamChunk(chunk.chunk);
                    }
                    if (chunk.is_final)
                        finishAssistantReply();
                },
                onError: (err) => setError(err instanceof Error ? err.message : "流式请求失败"),
                onComplete: () => finishAssistantReply(),
            });
        }
        catch (err) {
            setError(err instanceof Error ? err.message : "请求失败");
        }
    }
    return (_jsxs("div", { style: { height: "100%", width: "100%", minWidth: 0, display: "flex", overflow: "hidden" }, children: [_jsxs("aside", { style: {
                    width: 280,
                    flexShrink: 0,
                    borderRight: "1px solid var(--divider)",
                    display: "flex",
                    flexDirection: "column",
                    minHeight: 0,
                }, children: [_jsxs("div", { style: { padding: "14px 12px 10px", display: "flex", alignItems: "center", justifyContent: "space-between", gap: 10 }, children: [_jsx("div", { style: { fontSize: 16, fontWeight: 700 }, children: "AI \u5BF9\u8BDD" }), _jsx(GlassButton, { onClick: startNewConversation, style: { padding: "5px 10px", fontSize: 12 }, children: "\u65B0\u5EFA" })] }), _jsx("div", { style: { padding: "0 12px 10px" }, children: _jsxs("div", { style: {
                                display: "flex",
                                alignItems: "center",
                                gap: 8,
                                padding: "7px 8px",
                                borderRadius: 10,
                                background: "rgba(255,255,255,0.16)",
                                border: "1px solid rgba(255,255,255,0.26)",
                            }, children: [_jsx(ProviderAvatar, { modelName: selectedModel?.model_name, modelType: selectedModel?.model_type, size: 28 }), _jsx("select", { value: selectedModel ? modelKey(selectedModel) : "", disabled: modelsLoading || availableModels.length === 0 || streaming, onChange: (event) => setSelectedModelKey(event.target.value), title: compactModelName(selectedModel), style: {
                                        minWidth: 0,
                                        flex: 1,
                                        border: "none",
                                        outline: "none",
                                        background: "transparent",
                                        color: "var(--text-primary)",
                                        fontSize: 12,
                                        fontWeight: 700,
                                        cursor: modelsLoading || streaming ? "default" : "pointer",
                                    }, children: availableModels.length === 0 ? (_jsx("option", { value: "", children: modelsLoading ? "加载中" : "默认模型" })) : (availableModels.map((model) => (_jsx("option", { value: modelKey(model), children: modelDisplay(model) }, modelKey(model))))) })] }) }), _jsx(GlassScrollArea, { style: { flex: 1, minHeight: 0, padding: "0 8px 10px" }, children: orderedConversations.map((item) => {
                            const active = item.id === selectedConversationId;
                            const lastMessage = item.messages.length > 0 ? item.messages[item.messages.length - 1] : undefined;
                            const preview = lastMessage?.content ?? "空对话";
                            return (_jsxs("button", { onClick: () => setSelectedConversationId(item.id), style: {
                                    width: "100%",
                                    border: "none",
                                    borderRadius: 10,
                                    padding: "10px 12px",
                                    marginBottom: 6,
                                    textAlign: "left",
                                    cursor: "pointer",
                                    background: active ? "var(--tint-selected)" : "transparent",
                                    color: "var(--text-primary)",
                                }, children: [_jsxs("div", { style: { display: "flex", justifyContent: "space-between", gap: 8, alignItems: "center" }, children: [_jsx("span", { style: { minWidth: 0, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", fontWeight: 600, fontSize: 14 }, children: item.title }), _jsx("span", { style: { flexShrink: 0, fontSize: 11, color: "var(--text-disabled)" }, children: formatMessageTime(item.updatedAt) })] }), _jsx("div", { style: { marginTop: 4, fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: preview })] }, item.id));
                        }) })] }), _jsxs("section", { style: { minWidth: 0, flex: 1, display: "flex", flexDirection: "column", position: "relative" }, children: [_jsx(GlassScrollArea, { style: { flex: 1, minHeight: 0, padding: "18px 34px 10px" }, children: _jsxs("div", { style: { maxWidth: 1180, margin: "0 auto", width: "100%" }, children: [activeMessages.length === 0 && !streaming && (_jsxs("div", { style: {
                                        textAlign: "center",
                                        color: "var(--text-disabled)",
                                        fontSize: 14,
                                        paddingTop: 100,
                                        animation: "fade-up 300ms cubic-bezier(0.4,0,0.2,1) both",
                                    }, children: [_jsx("div", { style: {
                                                display: "inline-flex",
                                                alignItems: "center",
                                                justifyContent: "center",
                                                width: 56,
                                                height: 56,
                                                borderRadius: 16,
                                                background: "var(--glass-fill)",
                                                backdropFilter: "blur(12px)",
                                                border: "1px solid var(--glass-stroke)",
                                                marginBottom: 14,
                                                color: "var(--text-secondary)",
                                            }, children: _jsx(AgentReadyIcon, {}) }), _jsx("div", { style: { fontWeight: 500, marginBottom: 4, color: "var(--text-secondary)" }, children: "AI \u52A9\u624B\u5DF2\u5C31\u7EEA" }), _jsx("div", { style: { fontSize: 13 }, children: "\u8F93\u5165\u6D88\u606F\u5F00\u59CB\u5BF9\u8BDD" })] })), activeMessages.map((msg, i) => (_jsx(MessageRow, { message: msg, userAvatar: profile?.icon, userName: userName }, `${i}-${msg.role}`))), streaming && currentReply && (_jsx(MessageRow, { message: {
                                        role: "assistant",
                                        content: currentReply,
                                        modelType: currentStreamModel?.modelType ?? selectedModel?.model_type ?? "",
                                        modelName: currentStreamModel?.modelName ?? selectedModel?.model_name ?? "",
                                    }, userAvatar: profile?.icon, userName: userName, streaming: true })), error && (_jsx("p", { style: {
                                        color: "var(--color-badge)",
                                        fontSize: 13,
                                        padding: "8px 10px",
                                        background: "rgba(232,65,65,0.08)",
                                        borderRadius: 8,
                                        border: "1px solid rgba(232,65,65,0.18)",
                                    }, children: error })), _jsx("div", { ref: bottomRef })] }) }), _jsx("div", { style: {
                            padding: "10px 24px 14px",
                            background: "var(--glass-fill)",
                            backdropFilter: "blur(16px)",
                            WebkitBackdropFilter: "blur(16px)",
                            borderTop: "1px solid var(--divider)",
                        }, children: _jsxs("div", { style: {
                                maxWidth: 820,
                                margin: "0 auto",
                                display: "flex",
                                gap: 10,
                                alignItems: "stretch",
                            }, children: [_jsx("div", { style: { flex: 1, minWidth: 0 }, children: _jsx(GlassTextField, { value: input, onChange: (e) => setInput(e.target.value), placeholder: "\u8F93\u5165\u6D88\u606F... (Enter \u53D1\u9001)", onKeyDown: (e) => {
                                            if (e.key === "Enter" && !e.shiftKey) {
                                                e.preventDefault();
                                                void sendMessage();
                                            }
                                        } }) }), _jsx(GlassButton, { variant: "primary", onClick: () => void sendMessage(), disabled: streaming || !input.trim(), children: streaming ? _jsx(Spinner, { size: 16, color: "#fff" }) : "发送" })] }) })] })] }));
}
