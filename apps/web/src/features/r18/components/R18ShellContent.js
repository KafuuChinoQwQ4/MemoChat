import { jsx as _jsx, jsxs as _jsxs } from "react/jsx-runtime";
/** R18ShellContent — source switcher + search view for the R18 gateway domain. */
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
function sourceTitle(source) {
    return source.title?.trim() || source.name?.trim() || source.id;
}
function R18Cover({ cover, title }) {
    const coverUrl = useMediaUrl(cover);
    if (!coverUrl) {
        return (_jsx("div", { style: { aspectRatio: "3 / 4", display: "grid", placeItems: "center", color: "var(--text-disabled)", background: "var(--tint-hover)" }, children: "\u65E0\u5C01\u9762" }));
    }
    return (_jsx("img", { src: coverUrl, alt: title ?? "cover", style: { width: "100%", aspectRatio: "3 / 4", objectFit: "cover", background: "var(--tint-hover)" } }));
}
export function R18ShellContent() {
    const uid = useSessionStore((s) => s.uid);
    const token = useSessionStore((s) => s.token);
    const [selectedSourceId, setSelectedSourceId] = useState("");
    const [keyword, setKeyword] = useState("");
    const [submittedKeyword, setSubmittedKeyword] = useState("");
    const [page, setPage] = useState(1);
    const [actionError, setActionError] = useState(null);
    const authReady = uid !== null && token !== null;
    const sourcesQuery = useQuery({
        queryKey: ["r18", "sources", uid],
        enabled: authReady,
        queryFn: async () => {
            if (uid === null || token === null)
                throw new Error("Missing R18 auth");
            const response = await getGateway().http.get(ENDPOINTS.r18Sources);
            if (response.error !== 0)
                throw new Error(response.message || `R18 sources failed: ${response.error}`);
            return response.data?.sources ?? [];
        },
    });
    const sources = useMemo(() => sourcesQuery.data ?? [], [sourcesQuery.data]);
    const selectedSource = useMemo(() => sources.find((source) => source.id === selectedSourceId) ?? sources.find((source) => source.enabled) ?? sources[0], [sources, selectedSourceId]);
    useEffect(() => {
        if (!selectedSourceId && selectedSource?.id) {
            setSelectedSourceId(selectedSource.id);
        }
    }, [selectedSource?.id, selectedSourceId]);
    const searchQuery = useQuery({
        queryKey: ["r18", "search", uid, selectedSource?.id, submittedKeyword, page],
        enabled: authReady && Boolean(selectedSource?.id),
        queryFn: async () => {
            if (uid === null || token === null || !selectedSource?.id)
                throw new Error("Missing R18 search input");
            const response = await getGateway().http.post(ENDPOINTS.r18Search, {
                source_id: selectedSource.id,
                keyword: submittedKeyword,
                page,
            });
            if (response.error !== 0)
                throw new Error(response.message || `R18 search failed: ${response.error}`);
            return response.data ?? { items: [] };
        },
    });
    async function toggleSource(source) {
        if (uid === null || token === null)
            return;
        const enable = !source.enabled;
        setActionError(null);
        try {
            const response = await getGateway().http.post(enable ? ENDPOINTS.r18SourceEnable : ENDPOINTS.r18SourceDisable, { source_id: source.id });
            if (response.error !== 0)
                throw new Error(response.message || "源切换失败");
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
    if (!authReady) {
        return (_jsx("div", { style: { height: "100%", width: "100%", display: "grid", placeItems: "center", color: "var(--text-disabled)" }, children: "\u767B\u5F55\u72B6\u6001\u65E0\u6548" }));
    }
    return (_jsxs("div", { style: { height: "100%", width: "100%", minWidth: 0, display: "grid", gridTemplateColumns: "280px minmax(0, 1fr)", overflow: "hidden" }, children: [_jsxs("aside", { style: { borderRight: "1px solid var(--divider)", display: "flex", flexDirection: "column", minHeight: 0 }, children: [_jsxs("div", { style: { padding: "14px 14px 10px" }, children: [_jsx("div", { style: { fontSize: 16, fontWeight: 700 }, children: "R18" }), _jsx("div", { style: { marginTop: 4, fontSize: 12, color: "var(--text-secondary)" }, children: "\u5185\u5BB9\u6E90" })] }), _jsx(GlassScrollArea, { style: { flex: 1, minHeight: 0, padding: "0 8px 12px" }, children: sourcesQuery.isLoading ? (_jsx("div", { style: { display: "grid", placeItems: "center", padding: 28 }, children: _jsx(Spinner, { size: 24 }) })) : sources.map((source) => {
                            const active = source.id === selectedSource?.id;
                            return (_jsxs("button", { onClick: () => {
                                    setSelectedSourceId(source.id);
                                    setPage(1);
                                }, style: {
                                    width: "100%",
                                    border: "none",
                                    borderRadius: 10,
                                    padding: "10px 12px",
                                    marginBottom: 6,
                                    textAlign: "left",
                                    cursor: "pointer",
                                    background: active ? "var(--tint-selected)" : "transparent",
                                    color: "var(--text-primary)",
                                }, children: [_jsxs("div", { style: { display: "flex", alignItems: "center", justifyContent: "space-between", gap: 8 }, children: [_jsx("span", { style: { minWidth: 0, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", fontWeight: 600 }, children: sourceTitle(source) }), _jsx("span", { style: {
                                                    flexShrink: 0,
                                                    width: 8,
                                                    height: 8,
                                                    borderRadius: "50%",
                                                    background: source.enabled ? "var(--color-brand-green)" : "var(--text-disabled)",
                                                } })] }), _jsx("div", { style: { marginTop: 4, fontSize: 12, color: "var(--text-secondary)" }, children: source.version || source.status || source.id })] }, source.id));
                        }) })] }), _jsxs("section", { style: { minWidth: 0, display: "flex", flexDirection: "column", overflow: "hidden" }, children: [_jsxs("div", { style: { padding: "14px 18px", borderBottom: "1px solid var(--divider)", display: "flex", gap: 10, alignItems: "center" }, children: [_jsx("div", { style: { flex: 1, minWidth: 0 }, children: _jsx(GlassTextField, { value: keyword, onChange: (e) => setKeyword(e.target.value), placeholder: "\u641C\u7D22\u5173\u952E\u8BCD", onKeyDown: (e) => {
                                        if (e.key === "Enter") {
                                            e.preventDefault();
                                            submitSearch();
                                        }
                                    } }) }), _jsx(GlassButton, { onClick: submitSearch, variant: "primary", children: "\u641C\u7D22" }), selectedSource && (_jsx(GlassButton, { onClick: () => { void toggleSource(selectedSource); }, children: selectedSource.enabled ? "停用源" : "启用源" }))] }), (sourcesQuery.error || searchQuery.error || actionError) && (_jsx("div", { style: { margin: "12px 18px 0", color: "var(--color-badge)", fontSize: 13 }, children: actionError ||
                            (sourcesQuery.error instanceof Error ? sourcesQuery.error.message : null) ||
                            (searchQuery.error instanceof Error ? searchQuery.error.message : "加载失败") })), _jsx(GlassScrollArea, { style: { flex: 1, minHeight: 0, padding: 18 }, children: searchQuery.isLoading ? (_jsx("div", { style: { display: "grid", placeItems: "center", height: "100%" }, children: _jsx(Spinner, { size: 28 }) })) : (_jsxs("div", { style: { display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(190px, 1fr))", gap: 14 }, children: [(searchQuery.data?.items ?? []).map((item) => {
                                    return (_jsxs(GlassSurface, { elevated: true, style: { overflow: "hidden" }, children: [_jsx(R18Cover, { ...(item.cover === undefined ? {} : { cover: item.cover }), ...(item.title === undefined ? {} : { title: item.title }) }), _jsxs("div", { style: { padding: 12 }, children: [_jsx("div", { style: { fontWeight: 700, fontSize: 14, lineHeight: 1.35, overflow: "hidden", display: "-webkit-box", WebkitLineClamp: 2, WebkitBoxOrient: "vertical" }, children: item.title || item.comic_id || "未命名" }), (item.subtitle || item.author) && (_jsx("div", { style: { marginTop: 6, fontSize: 12, color: "var(--text-secondary)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }, children: item.subtitle || item.author }))] })] }, `${item.source_id}-${item.comic_id}`));
                                }), (searchQuery.data?.items ?? []).length === 0 && !searchQuery.isLoading && (_jsx(GlassSurface, { style: { minHeight: 220, display: "grid", placeItems: "center", color: "var(--text-disabled)", gridColumn: "1 / -1" }, children: "\u6682\u65E0\u7ED3\u679C" }))] })) })] })] }));
}
