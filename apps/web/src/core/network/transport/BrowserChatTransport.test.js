import { describe, expect, it } from "vitest";
import { BrowserChatTransport, } from "./BrowserChatTransport";
class FakeTransport {
    constructor() {
        this.connectCalls = [];
        this.sendCalls = [];
        this.closeCalls = 0;
        this.connected = false;
    }
    connect(info) {
        this.connectCalls.push(info);
    }
    send(reqId, payload) {
        this.sendCalls.push({ reqId, payload });
    }
    close() {
        this.closeCalls += 1;
    }
    isConnected() {
        return this.connected;
    }
    triggerError(err) {
        this.onError?.(err);
    }
    triggerClose(reason) {
        this.onClose?.(reason);
    }
}
function serverInfo(overrides = {}) {
    return {
        transport: "websocket",
        wsUrl: "ws://localhost:8444/ws",
        loginTicket: "ticket",
        token: "token",
        uid: 42,
        protocolVersion: 3,
        serverName: "chatserver1",
        websocketServerName: "chatserver1",
        ...overrides,
    };
}
describe("BrowserChatTransport", () => {
    it("uses websocket transport for the default browser path", () => {
        const websocket = new FakeTransport();
        const webtransport = new FakeTransport();
        const selector = new BrowserChatTransport({
            createWebSocketTransport: () => websocket,
            createWebTransportTransport: () => webtransport,
            isWebTransportSupported: () => true,
        });
        selector.connect(serverInfo());
        expect(websocket.connectCalls).toHaveLength(1);
        expect(websocket.connectCalls[0]?.transport).toBe("websocket");
        expect(webtransport.connectCalls).toHaveLength(0);
    });
    it("uses webtransport only when requested and browser support is present", () => {
        const websocket = new FakeTransport();
        const webtransport = new FakeTransport();
        const selector = new BrowserChatTransport({
            createWebSocketTransport: () => websocket,
            createWebTransportTransport: () => webtransport,
            isWebTransportSupported: () => true,
        });
        selector.connect(serverInfo({
            transport: "webtransport",
            wtUrl: "https://localhost:8445/chat",
            webtransportServerName: "chatserver1",
        }));
        expect(webtransport.connectCalls).toHaveLength(1);
        expect(webtransport.connectCalls[0]?.transport).toBe("webtransport");
        expect(websocket.connectCalls).toHaveLength(0);
    });
    it("falls back to websocket and logs when webtransport is unsupported", () => {
        const websocket = new FakeTransport();
        const logs = [];
        const selector = new BrowserChatTransport({
            createWebSocketTransport: () => websocket,
            isWebTransportSupported: () => false,
            logFallback: (reason) => logs.push(reason),
        });
        selector.connect(serverInfo({
            transport: "webtransport",
            wtUrl: "https://localhost:8445/chat",
            webtransportServerName: "chatserver1",
        }));
        expect(websocket.connectCalls).toHaveLength(1);
        expect(websocket.connectCalls[0]?.transport).toBe("websocket");
        expect(logs).toEqual(["webtransport_unsupported"]);
    });
    it("falls back to websocket when webtransport fails before ready", () => {
        const websocket = new FakeTransport();
        const webtransport = new FakeTransport();
        const logs = [];
        const selector = new BrowserChatTransport({
            createWebSocketTransport: () => websocket,
            createWebTransportTransport: () => webtransport,
            isWebTransportSupported: () => true,
            logFallback: (reason) => logs.push(reason),
        });
        selector.connect(serverInfo({
            transport: "webtransport",
            wtUrl: "https://localhost:8445/chat",
            webtransportServerName: "chatserver1",
        }));
        webtransport.triggerError(new Error("h3 failed"));
        expect(webtransport.closeCalls).toBe(1);
        expect(websocket.connectCalls).toHaveLength(1);
        expect(websocket.connectCalls[0]?.transport).toBe("websocket");
        expect(logs).toEqual(["webtransport_error_before_ready"]);
    });
});
