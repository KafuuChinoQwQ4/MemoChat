import { runtimeConfig } from "@/core/config/runtimeConfig";
import { useSessionStore } from "@/core/session/sessionStore";
import { throwForStatus } from "@/core/network/http/httpErrors";
import { ENDPOINTS } from "@/core/config/endpoints";
export async function uploadMedia(req) {
    const token = useSessionStore.getState().token;
    const form = new FormData();
    form.append("file", req.file);
    const res = await fetch(`${runtimeConfig.gateBaseUrl}${ENDPOINTS.mediaUpload}`, {
        method: "POST",
        headers: token ? { Authorization: `Bearer ${token}` } : {},
        body: form,
    });
    throwForStatus(res);
    return res.json();
}
