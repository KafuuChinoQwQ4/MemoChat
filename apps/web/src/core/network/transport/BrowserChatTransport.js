import { logger } from "@/core/common/logger";
import { WebSocketChatTransport } from "./WebSocketChatTransport";
import { WebTransportChatTransport } from "./WebTransportChatTransport";
const DEFAULT_WEBTRANSPORT_FALLBACK_TIMEOUT_MS = 3000;
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
        webTransportFallbackTimeoutMs: deps.webTransportFallbackTimeoutMs ?? DEFAULT_WEBTRANSPORT_FALLBACK_TIMEOUT_MS,
    };
}
export class BrowserChatTransport {
    constructor(deps = {}) {
        this._active = null;
        this._fallbackStarted = false;
        this._fallbackTimer = null;
        this._deps = resolveDeps(deps);
    }
    connect(info) {
        this._fallbackStarted = false;
        this._clearFallbackTimer();
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
        this._clearFallbackTimer();
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
        if (allowFallback) {
            this._startFallbackTimer(info);
        }
        transport.onReady = () => {
            this._clearFallbackTimer();
            this.onReady?.();
        };
        transport.onMessage = (reqId, payload) => this.onMessage?.(reqId, payload);
        transport.onClose = (reason) => {
            if (allowFallback &&
                !transport.isConnected() &&
                this._tryFallback(info, "webtransport_closed_before_ready")) {
                return;
            }
            this._clearFallbackTimer();
            this.onClose?.(reason);
        };
        transport.onError = (err) => {
            if (allowFallback &&
                !transport.isConnected() &&
                this._tryFallback(info, "webtransport_error_before_ready")) {
                return;
            }
            this._clearFallbackTimer();
            this.onError?.(err);
        };
        transport.connect(info);
    }
    _startFallbackTimer(info) {
        this._clearFallbackTimer();
        if (!info.wsUrl || this._deps.webTransportFallbackTimeoutMs <= 0) {
            return;
        }
        this._fallbackTimer = setTimeout(() => {
            if (this._active?.isConnected())
                return;
            this._tryFallback(info, "webtransport_ready_timeout");
        }, this._deps.webTransportFallbackTimeoutMs);
    }
    _clearFallbackTimer() {
        if (this._fallbackTimer !== null) {
            clearTimeout(this._fallbackTimer);
            this._fallbackTimer = null;
        }
    }
    _tryFallback(info, reason) {
        if (this._fallbackStarted || !info.wsUrl) {
            return false;
        }
        this._clearFallbackTimer();
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
