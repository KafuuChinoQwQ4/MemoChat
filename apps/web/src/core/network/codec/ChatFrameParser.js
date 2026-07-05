/**
 * ChatFrameParser — mirrors ChatFrameParser::append() state machine.
 *
 * Handles TCP-style fragmentation: a single WebSocket binary message may
 * contain partial headers, multiple frames, or a single frame split across
 * two messages. Call append() with each incoming ArrayBuffer; collect all
 * returned ChatFrame objects.
 */
import { CHAT_FRAME_HEADER_LEN } from "@/core/network/opcodes/protocol";
const dec = new TextDecoder();
export class ChatFrameParser {
    constructor() {
        this._buffer = new Uint8Array(0);
        this._pending = false;
        this._messageId = 0;
        this._messageLen = 0;
    }
    /**
     * Feed raw bytes into the parser.
     * @returns zero or more complete frames extracted from the buffer.
     */
    append(bytes) {
        // Grow the internal buffer
        const incoming = new Uint8Array(bytes);
        const merged = new Uint8Array(this._buffer.length + incoming.length);
        merged.set(this._buffer, 0);
        merged.set(incoming, this._buffer.length);
        this._buffer = merged;
        const frames = [];
        while (true) {
            if (!this._pending) {
                if (this._buffer.length < CHAT_FRAME_HEADER_LEN)
                    break;
                const view = new DataView(this._buffer.buffer, this._buffer.byteOffset, this._buffer.byteLength);
                this._messageId = view.getUint16(0, false /* big-endian */);
                this._messageLen = view.getUint16(2, false);
                this._buffer = this._buffer.slice(CHAT_FRAME_HEADER_LEN);
                this._pending = true;
            }
            if (this._buffer.length < this._messageLen)
                break;
            const payloadBytes = this._buffer.slice(0, this._messageLen);
            this._buffer = this._buffer.slice(this._messageLen);
            this._pending = false;
            frames.push({
                reqId: this._messageId,
                length: this._messageLen,
                payload: dec.decode(payloadBytes),
            });
            this._messageId = 0;
            this._messageLen = 0;
        }
        return frames;
    }
    reset() {
        this._buffer = new Uint8Array(0);
        this._pending = false;
        this._messageId = 0;
        this._messageLen = 0;
    }
}
