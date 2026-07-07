import { useEffect, useMemo, useState } from "react";
import { absoluteMediaUrl, isAuthorizedMediaUrl, resolveMediaUrl } from "@/shared/media/mediaUrl";
import { useSessionStore } from "@/core/session/sessionStore";
/** Resolves media URLs and fetches protected media with Authorization headers. */
export function useMediaUrl(ref) {
    const token = useSessionStore((s) => s.token);
    const resolved = useMemo(() => (ref ? resolveMediaUrl(ref) : ""), [ref]);
    const initialUrl = useMemo(() => (resolved && !isAuthorizedMediaUrl(resolved) ? resolved : ""), [resolved]);
    const [url, setUrl] = useState(initialUrl);
    useEffect(() => {
        if (!resolved) {
            setUrl("");
            return;
        }
        if (!isAuthorizedMediaUrl(resolved)) {
            setUrl(resolved);
            return;
        }
        if (!token || typeof URL.createObjectURL !== "function") {
            setUrl("");
            return;
        }
        const controller = new AbortController();
        let objectUrl = "";
        setUrl("");
        // Use `resolved` directly (relative or absolute) so the Vite dev proxy
        // can intercept relative paths like /media/download — avoids the
        // http://memochat.local fallback base that absoluteMediaUrl would add
        // when VITE_MEDIA_BASE_URL is unset.
        fetch(resolved, {
            headers: { Authorization: `Bearer ${token}` },
            signal: controller.signal,
        })
            .then((response) => {
            if (!response.ok)
                throw new Error(`media fetch failed: ${response.status}`);
            return response.blob();
        })
            .then((blob) => {
            if (controller.signal.aborted)
                return;
            objectUrl = URL.createObjectURL(blob);
            setUrl(objectUrl);
        })
            .catch(() => {
            if (!controller.signal.aborted)
                setUrl("");
        });
        return () => {
            controller.abort();
            if (objectUrl)
                URL.revokeObjectURL(objectUrl);
        };
    }, [resolved, token]);
    return url;
}
