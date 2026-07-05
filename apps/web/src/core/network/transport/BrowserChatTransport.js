import { logger } from "@/core/common/logger";
import { WebSocketChatTransport } from "./WebSocketChatTransport";
import { WebTransportChatTransport } from "./WebTransportChatTransport";
function defaultFallbackLogger(reason, info) {
    logger.transport.warn("webtransport.fallback_to_websocket", {
        reason,
        uid: info.uid,
        serverName: info.serverName,
        websocketServerName: info.websocketServerName,
        webtransportServerName: info.webtransportServerName,
    });
}
function resolveDeps(deps) {
    return {
        createWebSocketTransport: deps.createWebSocketTransport ?? (() => new WebSocketChatTransport()),
        createWebTransportTransport: deps.createWebTransportTransport ?? (() => new WebTransportChatTransport()),
        isWebTransportSupported: deps.isWebTransportSupported ?? (() => WebTransportChatTransport.isSupported()),
        logFallback: deps.logFallback ?? defaultFallbackLogger,
    };
}
export class BrowserChatTransport {
    constructor(deps = {}) {
        this._active = null;
        this._fallbackStarted = false;
        this._deps = resolveDeps(deps);
    }
    connect(info) {
        this._fallbackStarted = false;
        if (info.transport === "webtransport" && info.wtUrl) {
            if (this._deps.isWebTransportSupported()) {
                this._connectWith(this._deps.createWebTransportTransport(), info, true);
                return;
            }
            if (info.wsUrl) {
                this._deps.logFallback("webtransport_unsupported", info);
            }
        }
        this._connectWebSocket(info);
    }
    send(reqId, payload) {
        this._active?.send(reqId, payload);
    }
    close() {
        this._active?.close();
        this._active = null;
    }
    isConnected() {
        return this._active?.isConnected() ?? false;
    }
    _connectWebSocket(info) {
        this._connectWith(this._deps.createWebSocketTransport(), { ...info, transport: "websocket" }, false);
    }
    _connectWith(transport, info, allowFallback) {
        this._active?.close();
        this._active = transport;
        transport.onReady = () => this.onReady?.();
        transport.onMessage = (reqId, payload) => this.onMessage?.(reqId, payload);
        transport.onClose = (reason) => {
            if (allowFallback &&
                !transport.isConnected() &&
                this._tryFallback(info, "webtransport_closed_before_ready")) {
                return;
            }
            this.onClose?.(reason);
        };
        transport.onError = (err) => {
            if (allowFallback &&
                !transport.isConnected() &&
                this._tryFallback(info, "webtransport_error_before_ready")) {
                return;
            }
            this.onError?.(err);
        };
        transport.connect(info);
    }
    _tryFallback(info, reason) {
        if (this._fallbackStarted || !info.wsUrl) {
            return false;
        }
        this._fallbackStarted = true;
        this._deps.logFallback(reason, info);
        const previous = this._active;
        this._active = null;
        if (previous) {
            delete previous.onClose;
            delete previous.onError;
            previous.close();
        }
        this._connectWebSocket(info);
        return true;
    }
}
