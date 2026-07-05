/**
 * MockChatTransport — same interface as WebSocketChatTransport.
 *
 * Enabled via VITE_USE_MOCK_TRANSPORT=1.
 * Replays a queue of ArrayBuffer frames (fixed test fixtures) as if they arrived
 * from the real transport. Allows full chat feature development without a live server.
 */
import { encodeChatFrame } from "@/core/network/codec/ChatFrameCodec";
import { ChatFrameParser } from "@/core/network/codec/ChatFrameParser";
import { ReqId } from "@/core/network/opcodes/reqIds";
export class MockChatTransport {
    constructor(
    /** Optional frame queue to replay after chat-login ack. */
    _replayFrames = [], 
    /** Simulated network round-trip delay in ms */
    _delayMs = 80) {
        this._replayFrames = _replayFrames;
        this._delayMs = _delayMs;
        this._parser = new ChatFrameParser();
        this._connected = false;
    }
    connect(_info) {
        this._connected = false;
        setTimeout(() => {
            // Simulate chat-login ack
            const loginRsp = encodeChatFrame(ReqId.ID_CHAT_LOGIN_RSP, JSON.stringify({ error: 0 }));
            const frames = this._parser.append(loginRsp);
            for (const f of frames) {
                if (f.reqId === ReqId.ID_CHAT_LOGIN_RSP) {
                    this._connected = true;
                    this.onReady?.();
                }
                this.onMessage?.(f.reqId, f.payload);
            }
            // Replay any pre-loaded frames
            for (const buf of this._replayFrames) {
                setTimeout(() => {
                    for (const f of this._parser.append(buf)) {
                        this.onMessage?.(f.reqId, f.payload);
                    }
                }, this._delayMs);
            }
        }, this._delayMs);
    }
    send(_reqId, _payload) {
        // Swallow sends in mock mode; individual tests can spy on this method.
    }
    close() {
        this._connected = false;
        this.onClose?.("mock_closed");
    }
    isConnected() { return this._connected; }
    /**
     * Inject a frame as if it arrived from the server.
     * Useful in test setup to drive specific states.
     */
    injectFrame(reqId, payload) {
        const buf = encodeChatFrame(reqId, payload);
        for (const f of this._parser.append(buf)) {
            this.onMessage?.(f.reqId, f.payload);
        }
    }
}
