/**
 * ClientGateway — facade over all infrastructure services.
 * Mirrors the C++ ClientGateway struct.
 * Constructed once in AppProviders, distributed via DI.
 */
import { HttpClient } from "@/core/network/http/HttpClient";
import { BrowserChatTransport } from "@/core/network/transport/BrowserChatTransport";
import { MockChatTransport } from "@/core/network/transport/MockChatTransport";
import { ChatMessageDispatcher } from "@/core/network/dispatcher/ChatMessageDispatcher";
import { SseStreamClient } from "@/core/network/sse/SseStreamClient";
import { runtimeConfig } from "@/core/config/runtimeConfig";
import { useSessionStore } from "@/core/session/sessionStore";
export class ClientGateway {
    constructor() {
        this.dispatcher = new ChatMessageDispatcher();
        this.chatTransport = buildTransport();
        this.chatTransport.onMessage = (reqId, payload) => {
            this.dispatcher.dispatch({ reqId, length: payload.length, payload });
        };
        this.http = new HttpClient(runtimeConfig.gateBaseUrl, () => useSessionStore.getState().token);
        this.sse = new SseStreamClient();
    }
    get session() { return useSessionStore.getState(); }
}
function buildTransport() {
    if (runtimeConfig.useMockTransport)
        return new MockChatTransport();
    return new BrowserChatTransport();
}
/** Singleton — created once when AppProviders mounts */
let _gateway = null;
export function getGateway() {
    if (!_gateway)
        _gateway = new ClientGateway();
    return _gateway;
}
export function resetGateway() {
    _gateway?.chatTransport.close();
    _gateway = null;
}
