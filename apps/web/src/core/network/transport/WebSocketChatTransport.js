/**
 * WebSocketChatTransport — connects to the ChatServer WebSocket ingress.
 *
 * Frame protocol: binary, ArrayBuffer, same uint16+uint16+JSON encoding as the C++ client.
 * Owns heartbeat (1023 every 25 s) + watchdog (2 missed 1024 acks → reconnect).
 */
import { encodeChatFrame } from "@/core/network/codec/ChatFrameCodec";
import { ChatFrameParser } from "@/core/network/codec/ChatFrameParser";
import { HEARTBEAT_INTERVAL_MS, HEARTBEAT_MAX_MISS, RECONNECT_BASE_DELAY_MS, } from "@/core/network/opcodes/protocol";
import { ReqId } from "@/core/network/opcodes/reqIds";
export class WebSocketChatTransport {
    constructor() {
        this._ws = null;
        this._parser = new ChatFrameParser();
        this._heartbeatTimer = null;
        this._missCount = 0;
        this._reconnectAttempts = 0;
        this._serverInfo = null;
        this._connected = false;
        this._destroyed = false;
    }
    connect(info) {
        this._serverInfo = info;
        this._doConnect();
    }
    _doConnect() {
        if (!this._serverInfo || this._destroyed)
            return;
        if (!this._serverInfo.wsUrl) {
            this.onError?.(new Error("WebSocket chat endpoint is missing"));
            return;
        }
        this._parser.reset();
        this._connected = false;
        try {
            const ws = new WebSocket(this._serverInfo.wsUrl);
            ws.binaryType = "arraybuffer";
            this._ws = ws;
            ws.onopen = () => {
                if (this._destroyed) {
                    ws.close();
                    return;
                }
                // Send chat login (1005) immediately on WS open
                this.send(ReqId.ID_CHAT_LOGIN, JSON.stringify({
                    protocol_version: this._serverInfo.protocolVersion,
                    login_ticket: this._serverInfo.loginTicket,
                }));
            };
            ws.onmessage = (ev) => {
                const frames = this._parser.append(ev.data);
                for (const frame of frames) {
                    // Handle heartbeat ack inline to reset miss counter
                    if (frame.reqId === ReqId.ID_HEARTBEAT_RSP) {
                        this._missCount = 0;
                    }
                    // Handle chat-login ack: transition to ready
                    if (frame.reqId === ReqId.ID_CHAT_LOGIN_RSP) {
                        this._connected = true;
                        this._reconnectAttempts = 0;
                        this._startHeartbeat();
                        this.onReady?.();
                    }
                    this.onMessage?.(frame.reqId, frame.payload);
                }
            };
            ws.onclose = (ev) => {
                this._cleanup();
                if (!this._destroyed) {
                    this.onClose?.(ev.reason || "ws_closed");
                    this._scheduleReconnect();
                }
            };
            ws.onerror = (ev) => {
                this.onError?.(ev);
            };
        }
        catch (err) {
            this.onError?.(err);
            this._scheduleReconnect();
        }
    }
    send(reqId, payload) {
        if (!this._ws || this._ws.readyState !== WebSocket.OPEN)
            return;
        try {
            this._ws.send(encodeChatFrame(reqId, payload));
        }
        catch (err) {
            console.error("[WSTransport] send error", err);
        }
    }
    close() {
        this._destroyed = true;
        this._cleanup();
        this._ws?.close();
        this._ws = null;
    }
    isConnected() { return this._connected; }
    _startHeartbeat() {
        this._stopHeartbeat();
        this._missCount = 0;
        this._heartbeatTimer = setInterval(() => {
            if (!this._serverInfo)
                return;
            this._missCount++;
            if (this._missCount > HEARTBEAT_MAX_MISS) {
                console.warn("[WSTransport] heartbeat timeout — reconnecting");
                this._ws?.close();
                return;
            }
            this.send(ReqId.ID_HEART_BEAT_REQ, JSON.stringify({ fromuid: this._serverInfo.uid }));
        }, HEARTBEAT_INTERVAL_MS);
    }
    _stopHeartbeat() {
        if (this._heartbeatTimer !== null) {
            clearInterval(this._heartbeatTimer);
            this._heartbeatTimer = null;
        }
    }
    _cleanup() {
        this._connected = false;
        this._stopHeartbeat();
    }
    _scheduleReconnect() {
        if (this._destroyed)
            return;
        const delay = Math.min(RECONNECT_BASE_DELAY_MS * Math.pow(1.5, this._reconnectAttempts), 30000);
        this._reconnectAttempts++;
        setTimeout(() => this._doConnect(), delay);
    }
}
