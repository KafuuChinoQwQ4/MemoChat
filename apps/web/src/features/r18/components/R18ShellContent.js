import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** R18ShellContent — source switcher + search + chapter reader */
import { useEffect, useMemo, useState } from "react";
import { useQuery } from "@tanstack/react-query";
import { ENDPOINTS } from "@/core/config/endpoints";
import { useSessionStore } from "@/core/session/sessionStore";
import { getGateway } from "@/shared/gateway/ClientGateway";
import { useMediaUrl } from "@/shared/hooks/useMediaUrl";
import { GlassButton } from "@/shared/ui/glass/GlassButton";
import { GlassScrollArea } from "@/shared/ui/glass/GlassScrollArea";
import { GlassSurface } from "@/shared/ui/glass/GlassSurface";
import { GlassTextField } from "@/shared/ui/glass/GlassTextField";
import { Spinner } from "@/shared/ui/primitives/Spinner";
// ─── Helpers ──────────────────────────────────────────────────────────────
function sourceTitle(source) {
    return source.title?.trim() || source.name?.trim() || source.id;
}
// ─── Sub-components ───────────────────────────────────────────────────────
function R18Cover({ cover, title }) {
    const coverUrl = useMediaUrl(cover);
    return coverUrl ? (_jsx("img", { src: coverUrl, alt: title ?? "cover", loading: "lazy", style: { width: "100%", aspectRatio: "3 / 4", objectFit: "cover", display: "block", background: "var(--tint-hover)" } })) : (_jsx("div", { style: { aspectRatio: "3 / 4", display: "grid", placeItems: "center", color: "var(--text-disabled)", fontSize: 12, background: "var(--tint-hover)" }, children: "\u65E0\u5C01\u9762" }));
}
function CloseButton({ onClose }) {
    return (_jsx("button", { type: "button", "aria-label": "\u5173\u95ED", onClick: onClose, style: {
            width: 34, height: 34, flexShrink: 0,
            borderRadius: 10, border: "1px solid var(--divider)",
            background: "rgba(255,255,255,0.08)", color: "var(--text-secondary)",
            cursor: "pointer", fontSize: 18, lineHeight: 1,
            display: "grid", placeItems: "center",
        }, children: "\u00D7" }));
}
// ─── Chapter list overlay ─────────────────────────────────────────────────
function ChapterListOverlay({ comic, sourceId, onClose, onSelectChapter, }) {
    const chaptersQuery = useQuery({
        queryKey: ["r18", "chapters", sourceId, comic.comic_id],
        enabled: Boolean(sourceId && comic.comic_id),
        queryFn: async () => {
            const res = await getGateway().http.post(ENDPOINTS.r18ComicChapters, {
                source_id: sourceId,
                comic_id: comic.comic_id,
            });
            if (res.error !== 0)
                throw new Error(res.message || `获取章节失败: ${res.error}`);
            return res.data?.chapters ?? [];
        },
    });
    return (_jsx("div", { role: "dialog", "aria-modal": "true", onClick: onClose, style: {
            position: "fixed", inset: 0, zIndex: 90,
            display: "grid", placeItems: "center", padding: 24,
            background: "rgba(2, 6, 12, 0.62)",
            backdropFilter: "blur(14px)", WebkitBackdropFilter: "blur(14px)",
        }, children: _jsxs(GlassSurface, { elevated: true, onClick: (e) => e.stopPropagation(), style: {
                width: "min(560px, calc(100vw - 48px))",
                maxHeight: "min(680px, calc(100vh - 48px))",
                borderRadius: 18, overflow: "hidden",
                display: "flex", flexDirection: "column",
            }, children: [_jsxs("div", { style: {
                        padding: "16px 18px", display: "flex", alignItems: "center",
                        gap: 12, borderBottom: "1px solid var(--divider)", flexShrink: 0,
                    }, children: [_jsxs("div", { style: { flex: 1, minWidth: 0 }, children: [_jsx("div", { style: { fontWeight: 700, fontSize: 15, lineHeight: 1.3, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: comic.title || "未命名" }), comic.author && (_jsx("div", { style: { fontSize: 12, color: "var(--text-secondary)", marginTop: 3 }, children: comic.author }))] }), _jsx(CloseButton, { onClose: onClose })] }), _jsxs(GlassScrollArea, { style: { flex: 1, padding: "10px 12px 14px" }, children: [chaptersQuery.isLoading && (_jsx("div", { style: { display: "grid", placeItems: "center", padding: 40 }, children: _jsx(Spinner, { size: 26 }) })), chaptersQuery.error && (_jsx("div", { style: { padding: 20, color: "var(--color-badge)", fontSize: 13, textAlign: "center" }, children: chaptersQuery.error instanceof Error ? chaptersQuery.error.message : "加载失败" })), !chaptersQuery.isLoading && !chaptersQuery.error && (chaptersQuery.data ?? []).length === 0 && (_jsx("div", { style: { padding: 40, textAlign: "center", color: "var(--text-disabled)", fontSize: 13 }, children: "\u6682\u65E0\u7AE0\u8282" })), _jsx("div", { style: { display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(150px, 1fr))", gap: 8 }, children: (chaptersQuery.data ?? []).map((ch) => (_jsx("button", { onClick: () => onSelectChapter(ch.chapter_id, ch.title || `第 ${(ch.index ?? 0) + 1} 话`), style: {
                                    padding: "9px 12px", border: "1px solid var(--divider)",
                                    borderRadius: 9, background: "var(--glass-btn-bg)",
                                    color: "var(--text-primary)", fontSize: 13, textAlign: "left",
                                    cursor: "pointer", transition: "background 120ms ease",
                                    overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap",
                                }, onMouseEnter: (e) => { e.currentTarget.style.background = "var(--tint-hover)"; }, onMouseLeave: (e) => { e.currentTarget.style.background = "var(--glass-btn-bg)"; }, children: ch.title || `第 ${(ch.index ?? 0) + 1} 话` }, ch.chapter_id))) })] })] }) }));
}
// ─── Image reader overlay ─────────────────────────────────────────────────
function ReaderPage({ url }) {
    const loaded = useMediaUrl(url);
    if (!loaded)
        return (_jsx("div", { style: { width: "100%", minHeight: 200, display: "grid", placeItems: "center", background: "rgba(255,255,255,0.04)", borderRadius: 8 }, children: _jsx(Spinner, { size: 22 }) }));
    return (_jsx("img", { src: loaded, alt: "", loading: "lazy", style: { width: "100%", display: "block", borderRadius: 8 } }));
}
function ReaderOverlay({ comic, sourceId, chapterId, chapterTitle, onClose, }) {
    const imagesQuery = useQuery({
        queryKey: ["r18", "images", sourceId, comic.comic_id, chapterId],
        enabled: Boolean(sourceId && comic.comic_id && chapterId),
        queryFn: async () => {
            const res = await getGateway().http.post(ENDPOINTS.r18ChapterImages, {
                source_id: sourceId,
                comic_id: comic.comic_id,
                chapter_id: chapterId,
            });
            if (res.error !== 0)
                throw new Error(res.message || `获取图片失败: ${res.error}`);
            return res.data?.images ?? [];
        },
    });
    return (_jsxs("div", { role: "dialog", "aria-modal": "true", style: {
            position: "fixed", inset: 0, zIndex: 95,
            display: "flex", flexDirection: "column",
            background: "rgba(2, 6, 12, 0.88)",
            backdropFilter: "blur(18px)", WebkitBackdropFilter: "blur(18px)",
        }, children: [_jsxs("div", { style: {
                    display: "flex", alignItems: "center", gap: 12,
                    padding: "10px 18px", borderBottom: "1px solid rgba(255,255,255,0.10)",
                    background: "rgba(6,10,16,0.60)", flexShrink: 0,
                }, children: [_jsx("button", { type: "button", onClick: onClose, "aria-label": "\u8FD4\u56DE", style: {
                            display: "flex", alignItems: "center", gap: 6,
                            padding: "6px 12px", borderRadius: 9,
                            border: "1px solid rgba(255,255,255,0.14)",
                            background: "rgba(255,255,255,0.08)", color: "#dde4e2",
                            cursor: "pointer", fontSize: 13, flexShrink: 0,
                        }, children: "\u2190 \u8FD4\u56DE" }), _jsx("div", { style: { flex: 1, minWidth: 0 }, children: _jsxs("div", { style: { fontSize: 14, fontWeight: 600, color: "#f0f3f0", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: [comic.title || "未命名", " \u00B7 ", chapterTitle] }) })] }), _jsx(GlassScrollArea, { style: { flex: 1 }, children: _jsxs("div", { style: { maxWidth: 820, margin: "0 auto", padding: "20px 16px 32px", display: "flex", flexDirection: "column", gap: 12 }, children: [imagesQuery.isLoading && (_jsx("div", { style: { display: "grid", placeItems: "center", padding: 60 }, children: _jsx(Spinner, { size: 30 }) })), imagesQuery.error && (_jsx("div", { style: { padding: 40, color: "var(--color-badge)", fontSize: 14, textAlign: "center" }, children: imagesQuery.error instanceof Error ? imagesQuery.error.message : "加载失败" })), (imagesQuery.data ?? []).map((url, idx) => (_jsx(ReaderPage, { url: url }, `${chapterId}-${idx}`))), !imagesQuery.isLoading && !imagesQuery.error && (imagesQuery.data ?? []).length === 0 && (_jsx("div", { style: { padding: 60, textAlign: "center", color: "#aeb5bd", fontSize: 14 }, children: "\u672C\u7AE0\u6682\u65E0\u56FE\u7247" }))] }) })] }));
}
// ─── Main component ───────────────────────────────────────────────────────
export function R18ShellContent() {
    const uid = useSessionStore((s) => s.uid);
    const token = useSessionStore((s) => s.token);
    const [selectedSourceId, setSelectedSourceId] = useState("");
    const [keyword, setKeyword] = useState("");
    const [submittedKeyword, setSubmittedKeyword] = useState("");
    const [page, setPage] = useState(1);
    const [actionError, setActionError] = useState(null);
    /** Comic whose chapter list is open */
    const [comicForChapters, setComicForChapters] = useState(null);
    /** Chapter being read */
    const [readerState, setReaderState] = useState(null);
    const authReady = uid !== null && token !== null;
    const sourcesQuery = useQuery({
        queryKey: ["r18", "sources", uid],
        enabled: authReady,
        queryFn: async () => {
            if (uid === null || token === null)
                throw new Error("Missing R18 auth");
            const res = await getGateway().http.get(ENDPOINTS.r18Sources);
            if (res.error !== 0)
                throw new Error(res.message || `R18 sources failed: ${res.error}`);
            return res.data?.sources ?? [];
        },
    });
    const sources = useMemo(() => sourcesQuery.data ?? [], [sourcesQuery.data]);
    const selectedSource = useMemo(() => sources.find((s) => s.id === selectedSourceId) ?? sources.find((s) => s.enabled) ?? sources[0], [sources, selectedSourceId]);
    useEffect(() => {
        if (!selectedSourceId && selectedSource?.id)
            setSelectedSourceId(selectedSource.id);
    }, [selectedSource?.id, selectedSourceId]);
    const searchQuery = useQuery({
        queryKey: ["r18", "search", uid, selectedSource?.id, submittedKeyword, page],
        enabled: authReady && Boolean(selectedSource?.id),
        queryFn: async () => {
            if (uid === null || token === null || !selectedSource?.id)
                throw new Error("Missing R18 search input");
            const res = await getGateway().http.post(ENDPOINTS.r18Search, {
                source_id: selectedSource.id,
                keyword: submittedKeyword,
                page,
            });
            if (res.error !== 0)
                throw new Error(res.message || `R18 search failed: ${res.error}`);
            return res.data ?? { items: [] };
        },
    });
    async function toggleSource(source) {
        if (uid === null || token === null)
            return;
        setActionError(null);
        try {
            const res = await getGateway().http.post(source.enabled ? ENDPOINTS.r18SourceDisable : ENDPOINTS.r18SourceEnable, { source_id: source.id });
            if (res.error !== 0)
                throw new Error(res.message || "源切换失败");
            await sourcesQuery.refetch();
        }
        catch (err) {
            setActionError(err instanceof Error ? err.message : "源切换失败");
        }
    }
    function submitSearch() {
        setSubmittedKeyword(keyword.trim());
        setPage(1);
    }
    // ── Reader overlay (full-screen) ────────────────────────────────
    if (readerState) {
        return (_jsx(ReaderOverlay, { comic: readerState.comic, sourceId: selectedSource?.id ?? "", chapterId: readerState.chapterId, chapterTitle: readerState.chapterTitle, onClose: () => setReaderState(null) }));
    }
    if (!authReady) {
        return (_jsx("div", { style: { height: "100%", width: "100%", display: "grid", placeItems: "center", color: "var(--text-disabled)" }, children: "\u767B\u5F55\u72B6\u6001\u65E0\u6548" }));
    }
    return (_jsxs("div", { style: { height: "100%", width: "100%", minWidth: 0, display: "grid", gridTemplateColumns: "260px minmax(0, 1fr)", overflow: "hidden" }, children: [_jsxs("aside", { style: { borderRight: "1px solid var(--divider)", display: "flex", flexDirection: "column", minHeight: 0 }, children: [_jsxs("div", { style: { padding: "14px 14px 10px", flexShrink: 0 }, children: [_jsx("div", { style: { fontSize: 16, fontWeight: 700 }, children: "R18" }), _jsx("div", { style: { marginTop: 3, fontSize: 12, color: "var(--text-secondary)" }, children: "\u5185\u5BB9\u6E90" })] }), _jsx(GlassScrollArea, { style: { flex: 1, minHeight: 0, padding: "0 8px 12px" }, children: sourcesQuery.isLoading ? (_jsx("div", { style: { display: "grid", placeItems: "center", padding: 28 }, children: _jsx(Spinner, { size: 24 }) })) : sources.map((source) => {
                            const active = source.id === selectedSource?.id;
                            return (_jsxs("button", { onClick: () => { setSelectedSourceId(source.id); setPage(1); }, style: {
                                    width: "100%", border: "none", borderRadius: 10,
                                    padding: "10px 12px", marginBottom: 4, textAlign: "left",
                                    cursor: "pointer", transition: "background 120ms ease",
                                    background: active ? "var(--tint-selected)" : "transparent",
                                    color: "var(--text-primary)",
                                }, onMouseEnter: (e) => { if (!active)
                                    e.currentTarget.style.background = "var(--tint-hover)"; }, onMouseLeave: (e) => { if (!active)
                                    e.currentTarget.style.background = "transparent"; }, children: [_jsxs("div", { style: { display: "flex", alignItems: "center", justifyContent: "space-between", gap: 8 }, children: [_jsx("span", { style: { minWidth: 0, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", fontWeight: 600, fontSize: 14 }, children: sourceTitle(source) }), _jsx("span", { style: {
                                                    flexShrink: 0, width: 8, height: 8, borderRadius: "50%",
                                                    background: source.enabled ? "var(--color-brand-green)" : "var(--text-disabled)",
                                                } })] }), _jsx("div", { style: { marginTop: 3, fontSize: 12, color: "var(--text-secondary)" }, children: source.version || source.status || source.id })] }, source.id));
                        }) })] }), _jsxs("section", { style: { minWidth: 0, display: "flex", flexDirection: "column", overflow: "hidden" }, children: [_jsxs("div", { style: { padding: "12px 16px", borderBottom: "1px solid var(--divider)", display: "flex", gap: 8, alignItems: "center", flexShrink: 0 }, children: [_jsx("div", { style: { flex: 1, minWidth: 0 }, children: _jsx(GlassTextField, { value: keyword, onChange: (e) => setKeyword(e.target.value), placeholder: "\u641C\u7D22\u5173\u952E\u8BCD\u2026", onKeyDown: (e) => { if (e.key === "Enter") {
                                        e.preventDefault();
                                        submitSearch();
                                    } } }) }), _jsx(GlassButton, { onClick: submitSearch, variant: "primary", style: { flexShrink: 0 }, children: "\u641C\u7D22" }), selectedSource && (_jsx(GlassButton, { onClick: () => void toggleSource(selectedSource), style: { flexShrink: 0 }, children: selectedSource.enabled ? "停用源" : "启用源" }))] }), _jsxs("div", { style: { display: "flex", alignItems: "center", gap: 8, padding: "8px 16px", borderBottom: "1px solid var(--divider)", flexShrink: 0 }, children: [_jsx(GlassButton, { onClick: () => setPage((p) => Math.max(1, p - 1)), disabled: page <= 1, style: { padding: "5px 12px", fontSize: 12 }, children: "\u2190 \u4E0A\u4E00\u9875" }), _jsxs("span", { style: { fontSize: 13, color: "var(--text-secondary)", flex: 1, textAlign: "center" }, children: ["\u7B2C ", page, " \u9875"] }), _jsx(GlassButton, { onClick: () => setPage((p) => p + 1), disabled: !searchQuery.data?.max_page || page >= (searchQuery.data.max_page ?? 1), style: { padding: "5px 12px", fontSize: 12 }, children: "\u4E0B\u4E00\u9875 \u2192" })] }), (actionError || sourcesQuery.error || searchQuery.error) && (_jsx("div", { style: { margin: "10px 16px 0", color: "var(--color-badge)", fontSize: 13 }, children: actionError ||
                            (sourcesQuery.error instanceof Error ? sourcesQuery.error.message : null) ||
                            (searchQuery.error instanceof Error ? searchQuery.error.message : "加载失败") })), _jsx(GlassScrollArea, { style: { flex: 1, minHeight: 0, padding: 16 }, children: searchQuery.isLoading ? (_jsx("div", { style: { display: "grid", placeItems: "center", height: 200 }, children: _jsx(Spinner, { size: 28 }) })) : (_jsxs("div", { style: { display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(170px, 1fr))", gap: 12 }, children: [(searchQuery.data?.items ?? []).map((item) => (_jsxs(GlassSurface, { elevated: true, style: {
                                        overflow: "hidden", cursor: "pointer",
                                        transition: "transform 150ms ease, box-shadow 150ms ease",
                                    }, onClick: () => setComicForChapters(item), onMouseEnter: (e) => {
                                        const el = e.currentTarget;
                                        el.style.transform = "translateY(-3px)";
                                    }, onMouseLeave: (e) => {
                                        const el = e.currentTarget;
                                        el.style.transform = "";
                                    }, tabIndex: 0, role: "button", onKeyDown: (e) => { if (e.key === "Enter" || e.key === " ") {
                                        e.preventDefault();
                                        setComicForChapters(item);
                                    } }, children: [_jsx(R18Cover, { ...(item.cover !== undefined ? { cover: item.cover } : {}), ...(item.title !== undefined ? { title: item.title } : {}) }), _jsxs("div", { style: { padding: "10px 11px 12px" }, children: [_jsx("div", { style: {
                                                        fontWeight: 700, fontSize: 13, lineHeight: 1.35,
                                                        overflow: "hidden", display: "-webkit-box",
                                                        WebkitLineClamp: 2, WebkitBoxOrient: "vertical",
                                                    }, children: item.title || item.comic_id || "未命名" }), (item.subtitle || item.author) && (_jsx("div", { style: { marginTop: 5, fontSize: 11, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: item.subtitle || item.author }))] })] }, `${item.source_id}-${item.comic_id}`))), (searchQuery.data?.items ?? []).length === 0 && !searchQuery.isLoading && (_jsx("div", { style: {
                                        gridColumn: "1 / -1", padding: 60, textAlign: "center",
                                        color: "var(--text-disabled)", fontSize: 14,
                                    }, children: "\u6682\u65E0\u7ED3\u679C\uFF0C\u8BF7\u641C\u7D22\u5173\u952E\u8BCD" }))] })) })] }), comicForChapters && (_jsx(ChapterListOverlay, { comic: comicForChapters, sourceId: selectedSource?.id ?? "", onClose: () => setComicForChapters(null), onSelectChapter: (chapterId, chapterTitle) => {
                    setReaderState({ comic: comicForChapters, chapterId, chapterTitle });
                    setComicForChapters(null);
                } }))] }));
}
