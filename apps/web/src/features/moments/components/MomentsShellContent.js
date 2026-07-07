import { jsx as _jsx, jsxs as _jsxs, Fragment as _Fragment } from "react/jsx-runtime";
/** MomentsShellContent — moments feed */
import { useState } from "react";
import { useQuery, useQueryClient } from "@tanstack/react-query";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { ENDPOINTS } from "@/core/config/endpoints";
import { useSessionStore } from "@/core/session/sessionStore";
import { displayNameWithoutInternalId } from "@/core/entities/displayIds";
import { useMediaUrl } from "@/shared/hooks/useMediaUrl";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassTextField } from "@/shared/ui/glass/GlassTextField";
import { Avatar } from "@/shared/ui/primitives/Avatar";
import { Spinner } from "@/shared/ui/primitives/Spinner";
import { formatMessageTime } from "@/shared/lib/time";
const TEXT_PREVIEW_MAX_LINES = 7;
const TEXT_PREVIEW_LONG_THRESHOLD = 220;
const MEDIA_PREVIEW_MAX_HEIGHT = 420;
const MEDIA_BLOCK_GAP = 8;
function normalizeMomentMediaType(type) {
    const normalized = type?.trim().toLowerCase();
    if (normalized === "image" || normalized === "video")
        return normalized;
    return null;
}
function logicalLineCount(text) {
    if (!text)
        return 0;
    return text.split(/\r\n|\r|\n/).length;
}
function hasTextPreviewOverflow(text) {
    return text.length > TEXT_PREVIEW_LONG_THRESHOLD || logicalLineCount(text) > TEXT_PREVIEW_MAX_LINES;
}
function mediaDisplayHeight(media, full = false) {
    if (media.type === "video")
        return full ? 236 : 188;
    const sourceWidth = media.width > 0 ? media.width : 200;
    const sourceHeight = media.height > 0 ? media.height : 200;
    const aspect = sourceHeight / Math.max(1, sourceWidth);
    const baseWidth = full ? 420 : 220;
    const minHeight = full ? 140 : 90;
    const maxHeight = full ? 520 : 260;
    return Math.min(maxHeight, Math.max(minHeight, Math.round(baseWidth * aspect)));
}
function mediaStackHeight(media, full = false) {
    return media.reduce((total, item, index) => {
        const gap = index > 0 ? MEDIA_BLOCK_GAP : 0;
        return total + gap + mediaDisplayHeight(item, full);
    }, 0);
}
function hasMediaPreviewOverflow(media) {
    return mediaStackHeight(media) > MEDIA_PREVIEW_MAX_HEIGHT + 1;
}
function mapMoment(item) {
    const textParts = [];
    const media = [];
    (item.items ?? []).forEach((part, index) => {
        const type = normalizeMomentMediaType(part.media_type);
        const content = part.content?.trim() ?? "";
        const mediaKey = part.media_key?.trim() ?? "";
        const thumbKey = part.thumb_key?.trim() ?? "";
        const previewKey = thumbKey || mediaKey;
        if (type && previewKey) {
            media.push({
                id: `${item.moment_id}-${part.seq ?? index}-${previewKey}`,
                type,
                mediaKey: mediaKey || previewKey,
                previewKey,
                width: Number(part.width ?? 0),
                height: Number(part.height ?? 0),
                durationMs: Number(part.duration_ms ?? 0),
            });
            return;
        }
        if (content) {
            textParts.push(content);
        }
    });
    const textContent = textParts.join("\n");
    const authorName = displayNameWithoutInternalId(item.user_nick?.trim() || item.user_name?.trim(), item.user_id, item.uid, "未知用户");
    const location = item.location?.trim() ?? "";
    return {
        id: String(item.moment_id),
        authorUid: item.uid,
        authorName,
        ...(item.user_icon ? { authorIcon: item.user_icon } : {}),
        content: textContent || location,
        media,
        ...(location ? { location } : {}),
        createdAt: Number(item.created_at || 0),
        likeCount: Math.max(0, Number(item.like_count ?? 0)),
        commentCount: Math.max(0, Number(item.comment_count ?? 0)),
    };
}
function HeartIcon() {
    return (_jsx("svg", { width: "13", height: "13", viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: "1.8", strokeLinecap: "round", strokeLinejoin: "round", "aria-hidden": true, children: _jsx("path", { d: "M12 19.2s-6.8-4-8.2-8.2C2.9 8.3 4.4 6.1 7 6.1c1.5 0 2.7.8 3.4 2 .7-1.2 1.9-2 3.4-2 2.6 0 4.1 2.2 3.2 4.9C18.8 15.2 12 19.2 12 19.2Z" }) }));
}
function CommentIcon() {
    return (_jsxs("svg", { width: "13", height: "13", viewBox: "0 0 24 24", fill: "none", stroke: "currentColor", strokeWidth: "1.8", strokeLinecap: "round", strokeLinejoin: "round", "aria-hidden": true, children: [_jsx("path", { d: "M6.5 16.2h8.7c1.9 0 3.1-1.2 3.1-3V8.3c0-1.8-1.2-3-3.1-3H8.8c-1.9 0-3.1 1.2-3.1 3v4.8c0 1.2.5 2.1 1.5 2.7l-.4 2.1 2-1.7Z" }), _jsx("path", { d: "M8.6 9.4h6.5" }), _jsx("path", { d: "M8.6 12.2h4.2" })] }));
}
function previewTextStyle(text) {
    const style = {
        fontSize: 14,
        lineHeight: 1.65,
        margin: 0,
        color: "var(--text-primary)",
        whiteSpace: "pre-wrap",
        overflowWrap: "anywhere",
    };
    if (!hasTextPreviewOverflow(text))
        return style;
    return {
        ...style,
        display: "-webkit-box",
        WebkitLineClamp: TEXT_PREVIEW_MAX_LINES,
        WebkitBoxOrient: "vertical",
        overflow: "hidden",
    };
}
function formatDuration(durationMs) {
    if (!durationMs || durationMs <= 0)
        return "点击打开视频";
    const totalSec = Math.floor(durationMs / 1000);
    const minutes = Math.floor(totalSec / 60);
    const seconds = totalSec % 60;
    return `${minutes}:${seconds < 10 ? `0${seconds}` : seconds}`;
}
function MomentMediaTile({ media, full, onOpen, }) {
    const height = mediaDisplayHeight(media, full);
    const source = useMediaUrl(full ? media.mediaKey : media.previewKey);
    const previewSource = useMediaUrl(media.type === "video" ? media.previewKey : null);
    const interactive = Boolean(onOpen);
    const sharedStyle = {
        width: "100%",
        height,
        borderRadius: 10,
        overflow: "hidden",
        border: "1px solid var(--divider)",
        background: "rgba(255,255,255,0.08)",
        cursor: interactive ? "pointer" : undefined,
    };
    if (media.type === "image") {
        return (_jsx("div", { role: interactive ? "button" : undefined, tabIndex: interactive ? 0 : undefined, onClick: onOpen, onKeyDown: (event) => {
                if (!interactive)
                    return;
                if (event.key === "Enter" || event.key === " ") {
                    event.preventDefault();
                    onOpen?.();
                }
            }, style: sharedStyle, children: _jsx("img", { src: source, alt: "", loading: "lazy", style: { width: "100%", height: "100%", objectFit: "cover", display: "block" } }) }));
    }
    const tile = (_jsx("div", { style: {
            ...sharedStyle,
            position: "relative",
            display: "grid",
            placeItems: "center",
            color: "#eaf2ff",
            background: previewSource
                ? `linear-gradient(rgba(18,25,34,0.42), rgba(18,25,34,0.72)), url("${previewSource}") center/cover`
                : "linear-gradient(135deg, #263447, #111821)",
        }, children: _jsxs("div", { style: { textAlign: "center" }, children: [_jsx("div", { style: { fontSize: full ? 40 : 34, lineHeight: 1 }, children: "\u25B6" }), _jsx("div", { style: { fontSize: 12, marginTop: 8 }, children: formatDuration(media.durationMs) })] }) }));
    if (!full) {
        return (_jsx("div", { onClick: onOpen, style: { cursor: interactive ? "pointer" : undefined }, children: tile }));
    }
    return (_jsx("a", { href: source || undefined, target: "_blank", rel: "noreferrer", style: { textDecoration: "none" }, children: tile }));
}
function MomentMediaBlocks({ media, full = false, onOpen, }) {
    if (media.length === 0)
        return null;
    const overflow = !full && hasMediaPreviewOverflow(media);
    return (_jsxs("div", { style: {
            position: "relative",
            display: "grid",
            gap: MEDIA_BLOCK_GAP,
            marginTop: 12,
            maxHeight: full ? undefined : MEDIA_PREVIEW_MAX_HEIGHT,
            overflow: full ? "visible" : "hidden",
        }, children: [media.map((part) => (_jsx(MomentMediaTile, { media: part, full: full, ...(!full && onOpen ? { onOpen } : {}) }, part.id))), overflow ? (_jsx("button", { type: "button", onClick: onOpen, style: {
                    position: "absolute",
                    left: 0,
                    right: 0,
                    bottom: 0,
                    height: 72,
                    border: 0,
                    borderRadius: "0 0 10px 10px",
                    padding: "0 12px 12px",
                    display: "flex",
                    alignItems: "flex-end",
                    justifyContent: "flex-end",
                    color: "#86b7ff",
                    fontSize: 13,
                    cursor: "pointer",
                    background: "linear-gradient(rgba(20,24,32,0), rgba(20,24,32,0.78))",
                }, children: "\u67E5\u770B\u5168\u90E8" })) : null] }));
}
function MomentDetailOverlay({ moment, onClose }) {
    return (_jsx("div", { role: "dialog", "aria-modal": "true", onClick: onClose, style: {
            position: "fixed",
            inset: 0,
            zIndex: 80,
            padding: 24,
            display: "grid",
            placeItems: "center",
            background: "rgba(2, 6, 12, 0.58)",
            backdropFilter: "blur(14px)",
        }, children: _jsxs(GlassSurface, { elevated: true, onClick: (event) => event.stopPropagation(), style: {
                width: "min(760px, calc(100vw - 48px))",
                maxHeight: "min(780px, calc(100vh - 48px))",
                padding: 18,
                borderRadius: 18,
                overflow: "hidden",
                display: "flex",
                flexDirection: "column",
            }, children: [_jsxs("div", { style: { display: "flex", alignItems: "center", gap: 12, marginBottom: 14 }, children: [_jsx(Avatar, { src: moment.authorIcon, name: moment.authorName, size: 40 }), _jsxs("div", { style: { flex: 1, minWidth: 0 }, children: [_jsx("div", { style: { fontSize: 15, fontWeight: 700 }, children: moment.authorName }), _jsxs("div", { style: { fontSize: 12, color: "var(--text-disabled)", marginTop: 2 }, children: [formatMessageTime(moment.createdAt), moment.location ? ` · ${moment.location}` : ""] })] }), _jsx("button", { type: "button", onClick: onClose, "aria-label": "\u5173\u95ED", style: {
                                width: 34,
                                height: 34,
                                borderRadius: 10,
                                border: "1px solid var(--divider)",
                                color: "var(--text-secondary)",
                                background: "rgba(255,255,255,0.08)",
                                cursor: "pointer",
                                fontSize: 18,
                            }, children: "\u00D7" })] }), _jsxs("div", { style: { overflowY: "auto", paddingRight: 4 }, children: [moment.content ? (_jsx("p", { style: {
                                fontSize: 15,
                                lineHeight: 1.72,
                                whiteSpace: "pre-wrap",
                                overflowWrap: "anywhere",
                                color: "var(--text-primary)",
                                margin: 0,
                            }, children: moment.content })) : null, _jsx(MomentMediaBlocks, { media: moment.media, full: true })] })] }) }));
}
export function MomentsShellContent() {
    const uid = useSessionStore((s) => s.uid);
    const token = useSessionStore((s) => s.token);
    const queryClient = useQueryClient();
    const [selectedMoment, setSelectedMoment] = useState(null);
    const [draftText, setDraftText] = useState("");
    const [draftLocation, setDraftLocation] = useState("");
    const [visibility, setVisibility] = useState(0);
    const [publishing, setPublishing] = useState(false);
    const [publishStatus, setPublishStatus] = useState("");
    const { data, isLoading, error } = useQuery({
        queryKey: ["moments", "feed", uid],
        enabled: uid !== null && token !== null,
        queryFn: async () => {
            if (uid === null || token === null)
                throw new Error("Missing moments auth");
            const response = await getGateway().http.post(ENDPOINTS.momentsList, {
                last_moment_id: 0,
                limit: 20,
            });
            if (response.error !== 0)
                throw new Error(`Moments list failed: ${response.error}`);
            return (response.moments ?? []).map(mapMoment);
        },
    });
    async function publishMoment() {
        const content = draftText.trim();
        if (!content) {
            setPublishStatus("先写一点内容再发布");
            return;
        }
        if (uid === null || token === null) {
            setPublishStatus("登录状态无效，请重新登录");
            return;
        }
        setPublishing(true);
        setPublishStatus("正在发布...");
        try {
            const response = await getGateway().http.post(ENDPOINTS.momentsPublish, {
                visibility,
                location: draftLocation.trim(),
                items: [{
                        media_type: "text",
                        content,
                    }],
            });
            const errorCode = response.error ?? response.code ?? 0;
            if (errorCode !== 0) {
                setPublishStatus(`发布失败，错误码 ${errorCode}`);
                return;
            }
            setDraftText("");
            setDraftLocation("");
            setPublishStatus(response.moment_id ? `已发布动态 ${response.moment_id}` : "已发布");
            await queryClient.invalidateQueries({ queryKey: ["moments", "feed", uid] });
        }
        catch (err) {
            setPublishStatus(err instanceof Error ? err.message : "发布失败");
        }
        finally {
            setPublishing(false);
        }
    }
    if (isLoading) {
        return (_jsx("div", { style: { height: "100%", display: "flex", alignItems: "center", justifyContent: "center" }, children: _jsx(Spinner, { size: 30 }) }));
    }
    if (error) {
        return (_jsx("div", { style: {
                height: "100%", display: "flex", alignItems: "center",
                justifyContent: "center", color: "var(--color-badge)", fontSize: 14,
            }, children: "\u52A0\u8F7D\u5931\u8D25\uFF0C\u8BF7\u5237\u65B0\u91CD\u8BD5" }));
    }
    const items = data ?? [];
    return (_jsxs(GlassScrollArea, { style: { height: "100%", width: "100%", padding: "18px 14px 22px" }, children: [_jsxs("div", { style: { maxWidth: 1360, margin: "0 auto", width: "100%" }, children: [_jsx("div", { style: { marginBottom: 18, animation: "fade-up 200ms ease both" }, children: _jsx("h2", { style: { fontSize: 18, fontWeight: 700 }, children: "\u670B\u53CB\u5708" }) }), _jsxs(GlassSurface, { elevated: true, style: {
                            padding: 16,
                            marginBottom: 14,
                            borderRadius: 14,
                            animation: "fade-up 210ms ease both",
                        }, children: [_jsx("textarea", { value: draftText, onChange: (event) => setDraftText(event.target.value), placeholder: "\u53D1\u5E03\u670B\u53CB\u5708...", "aria-label": "\u670B\u53CB\u5708\u5185\u5BB9", maxLength: 4096, style: {
                                    width: "100%",
                                    minHeight: 82,
                                    resize: "vertical",
                                    border: 0,
                                    borderRadius: 12,
                                    padding: "10px 12px",
                                    background: "rgba(255,255,255,0.10)",
                                    color: "var(--text-primary)",
                                    font: "inherit",
                                    fontSize: 14,
                                    lineHeight: 1.6,
                                    outline: "1px solid var(--divider)",
                                    boxSizing: "border-box",
                                } }), _jsxs("div", { style: {
                                    display: "grid",
                                    gridTemplateColumns: "minmax(0, 1fr) 132px auto",
                                    gap: 10,
                                    alignItems: "center",
                                    marginTop: 10,
                                }, children: [_jsx(GlassTextField, { value: draftLocation, onChange: (event) => setDraftLocation(event.target.value), placeholder: "\u4F4D\u7F6E\uFF08\u53EF\u9009\uFF09", "aria-label": "\u670B\u53CB\u5708\u4F4D\u7F6E" }), _jsxs("select", { value: visibility, onChange: (event) => setVisibility(Number(event.target.value)), "aria-label": "\u670B\u53CB\u5708\u53EF\u89C1\u8303\u56F4", style: {
                                            height: 38,
                                            borderRadius: 10,
                                            border: "1px solid var(--divider)",
                                            background: "rgba(255,255,255,0.12)",
                                            color: "var(--text-primary)",
                                            padding: "0 10px",
                                            font: "inherit",
                                        }, children: [_jsx("option", { value: 0, children: "\u516C\u5F00" }), _jsx("option", { value: 1, children: "\u597D\u53CB\u53EF\u89C1" })] }), _jsx(GlassButton, { type: "button", variant: "primary", onClick: () => void publishMoment(), disabled: publishing || !draftText.trim(), children: publishing ? "发布中" : "发布" })] }), publishStatus ? (_jsx("div", { style: { marginTop: 8, fontSize: 12, color: "var(--text-secondary)" }, children: publishStatus })) : null] }), items.length === 0 ? (_jsx(GlassSurface, { elevated: true, style: {
                            minHeight: 240,
                            display: "grid",
                            placeItems: "center",
                            color: "var(--text-disabled)",
                            fontSize: 14,
                            animation: "fade-up 220ms ease both",
                        }, children: "\u6682\u65E0\u52A8\u6001" })) : (_jsx("div", { style: {
                            columnWidth: 236,
                            columnGap: 12,
                            width: "100%",
                        }, children: items.map((item, idx) => (_jsxs(GlassSurface, { style: {
                                display: "inline-block",
                                width: "100%",
                                breakInside: "avoid",
                                margin: "0 0 12px",
                                padding: "15px 16px",
                                borderRadius: 12,
                                boxShadow: "0 1px 3px rgba(0,0,0,0.05), 0 4px 14px rgba(0,0,0,0.06)",
                                transition: "transform 160ms ease, box-shadow 160ms ease",
                                animation: `fade-up ${180 + idx * 30}ms cubic-bezier(0.4,0,0.2,1) both`,
                            }, onMouseEnter: (e) => {
                                const el = e.currentTarget;
                                el.style.transform = "translateY(-2px)";
                                el.style.boxShadow = "0 2px 6px rgba(0,0,0,0.07), 0 8px 24px rgba(0,0,0,0.09)";
                            }, onMouseLeave: (e) => {
                                const el = e.currentTarget;
                                el.style.transform = "";
                                el.style.boxShadow = "0 1px 3px rgba(0,0,0,0.05), 0 4px 14px rgba(0,0,0,0.06)";
                            }, children: [_jsxs("div", { style: { display: "flex", gap: 10, marginBottom: 10, alignItems: "center" }, children: [_jsx(Avatar, { src: item.authorIcon, name: item.authorName, size: 36 }), _jsxs("div", { style: { flex: 1, minWidth: 0 }, children: [_jsx("div", { style: { fontWeight: 600, fontSize: 14, lineHeight: 1.3 }, children: item.authorName }), _jsx("div", { style: { fontSize: 11, color: "var(--text-disabled)", marginTop: 1 }, children: formatMessageTime(item.createdAt) })] })] }), item.content ? (_jsxs(_Fragment, { children: [_jsx("p", { style: previewTextStyle(item.content), children: item.content }), hasTextPreviewOverflow(item.content) ? (_jsx("button", { type: "button", onClick: () => setSelectedMoment(item), style: {
                                                marginTop: 8,
                                                padding: 0,
                                                border: 0,
                                                background: "transparent",
                                                color: "#86b7ff",
                                                fontSize: 13,
                                                cursor: "pointer",
                                            }, children: "\u67E5\u770B\u5168\u6587" })) : null] })) : item.media.length === 0 ? (_jsx("p", { style: { fontSize: 14, lineHeight: 1.65, margin: 0, color: "var(--text-disabled)" }, children: "\u5185\u5BB9\u672A\u4FDD\u5B58\uFF0C\u8BF7\u91CD\u65B0\u53D1\u5E03" })) : null, _jsx(MomentMediaBlocks, { media: item.media, onOpen: () => setSelectedMoment(item) }), _jsxs("div", { style: {
                                        marginTop: 12,
                                        paddingTop: 10,
                                        borderTop: "1px solid var(--divider)",
                                        fontSize: 12,
                                        color: "var(--text-secondary)",
                                        display: "flex",
                                        gap: 16,
                                    }, children: [_jsxs("span", { style: { display: "inline-flex", alignItems: "center", gap: 4 }, children: [_jsx(HeartIcon, {}), " ", item.likeCount] }), _jsxs("span", { style: { display: "inline-flex", alignItems: "center", gap: 4 }, children: [_jsx(CommentIcon, {}), " ", item.commentCount] })] })] }, item.id))) }))] }), selectedMoment ? (_jsx(MomentDetailOverlay, { moment: selectedMoment, onClose: () => setSelectedMoment(null) })) : null] }));
}
