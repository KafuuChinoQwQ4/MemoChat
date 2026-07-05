import { describe, expect, it } from "vitest";
import { buildEndpointUrl, selectBrowserChatEndpoint } from "./postLoginBootstrap";
function endpoint(transport, priority, overrides = {}) {
    return {
        transport,
        host: "localhost",
        port: transport === "webtransport" ? "8445" : "8444",
        path: transport === "webtransport" ? "/chat" : "/ws",
        tls: transport !== "websocket",
        server_name: "chatserver1",
        priority,
        ...overrides,
    };
}
describe("selectBrowserChatEndpoint", () => {
    it("prefers webtransport by default and keeps websocket fallback", () => {
        const selected = selectBrowserChatEndpoint([
            endpoint("webtransport", 1),
            endpoint("websocket", 2),
        ], true);
        expect(selected?.primary.transport).toBe("webtransport");
        expect(selected?.websocketFallback?.transport).toBe("websocket");
    });
    it("can keep websocket first when webtransport is not preferred", () => {
        const selected = selectBrowserChatEndpoint([
            endpoint("websocket", 2),
            endpoint("webtransport", 1),
        ], false);
        expect(selected?.primary.transport).toBe("websocket");
        expect(selected?.websocketFallback).toBeUndefined();
    });
    it("does not select a webtransport-only endpoint when webtransport is not preferred", () => {
        const selected = selectBrowserChatEndpoint([
            endpoint("tcp", 1),
            endpoint("webtransport", 2),
        ], false);
        expect(selected).toBeNull();
    });
    it("can select a webtransport-only endpoint when webtransport is preferred", () => {
        const selected = selectBrowserChatEndpoint([
            endpoint("tcp", 1),
            endpoint("webtransport", 2),
        ], true);
        expect(selected?.primary.transport).toBe("webtransport");
        expect(selected?.websocketFallback).toBeUndefined();
    });
});
describe("buildEndpointUrl", () => {
    it("allows ws for localhost development endpoints", () => {
        expect(buildEndpointUrl(endpoint("websocket", 1, { tls: false }))).toBe("ws://localhost:8444/ws");
        expect(buildEndpointUrl(endpoint("websocket", 1, { host: "127.0.0.1", tls: false }))).toBe("ws://127.0.0.1:8444/ws");
    });
    it("forces wss for non-local websocket endpoints even when the login response asks for tls false", () => {
        expect(buildEndpointUrl(endpoint("websocket", 1, { host: "chat.example.com", tls: false }))).toBe("wss://chat.example.com:8444/ws");
    });
    it("keeps webtransport endpoints on https", () => {
        expect(buildEndpointUrl(endpoint("webtransport", 1, { host: "chat.example.com", tls: false }))).toBe("https://chat.example.com:8445/chat");
    });
});
