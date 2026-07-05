/**
 * ChatFrameCodec — mirrors ChatFrameCodec.cpp (encodeChatFrame)
 *
 * Frame layout: | uint16 BE msgId | uint16 BE len | UTF-8 JSON payload |
 * len is the byte-length of the JSON payload (NOT including the 4-byte header).
 * Max payload: 65 535 bytes (uint16 max).
 */
import { CHAT_FRAME_HEADER_LEN, MAX_FRAME_PAYLOAD } from "@/core/network/opcodes/protocol"
import type { ReqId } from "@/core/network/opcodes/reqIds"

const enc = new TextEncoder()

/**
 * Encode a chat frame as an ArrayBuffer.
 * @throws RangeError if payload exceeds 65 535 bytes.
 */
export function encodeChatFrame(reqId: ReqId, payload: string): ArrayBuffer {
  const body = enc.encode(payload)
  if (body.byteLength > MAX_FRAME_PAYLOAD) {
    throw new RangeError(
      `Chat frame payload too large: ${body.byteLength} bytes (max ${MAX_FRAME_PAYLOAD})`,
    )
  }
  const buf = new ArrayBuffer(CHAT_FRAME_HEADER_LEN + body.byteLength)
  const view = new DataView(buf)
  view.setUint16(0, reqId as number, false /* big-endian */)
  view.setUint16(2, body.byteLength, false)
  new Uint8Array(buf, CHAT_FRAME_HEADER_LEN).set(body)
  return buf
}
