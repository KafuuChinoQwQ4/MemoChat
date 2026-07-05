/**
 * codec.test.ts — byte-level unit tests for ChatFrameCodec + ChatFrameParser.
 * Most critical tests in the project: protocol correctness.
 */
import { describe, it, expect, beforeEach } from "vitest"
import { encodeChatFrame } from "./ChatFrameCodec"
import { ChatFrameParser } from "./ChatFrameParser"
import { ReqId } from "@/core/network/opcodes/reqIds"

describe("encodeChatFrame", () => {
  it("encodes a minimal frame with correct header", () => {
    const buf = encodeChatFrame(ReqId.ID_HEART_BEAT_REQ, "{}")
    const view = new DataView(buf)
    expect(view.getUint16(0, false)).toBe(ReqId.ID_HEART_BEAT_REQ) // 1023
    expect(view.getUint16(2, false)).toBe(2) // len of "{}"
    const body = new TextDecoder().decode(new Uint8Array(buf, 4))
    expect(body).toBe("{}")
  })

  it("encodes a larger payload", () => {
    const payload = JSON.stringify({ uid: 12345, msg: "hello world" })
    const buf = encodeChatFrame(ReqId.ID_TEXT_CHAT_MSG_REQ, payload)
    const view = new DataView(buf)
    expect(view.getUint16(0, false)).toBe(ReqId.ID_TEXT_CHAT_MSG_REQ)
    expect(view.getUint16(2, false)).toBe(new TextEncoder().encode(payload).byteLength)
  })

  it("throws RangeError when payload exceeds uint16 max", () => {
    const huge = "x".repeat(65_536)
    expect(() => encodeChatFrame(ReqId.ID_HEART_BEAT_REQ, huge)).toThrow(RangeError)
  })

  it("correctly encodes unicode multibyte payload length", () => {
    const payload = JSON.stringify({ msg: "你好世界" }) // each CJK char = 3 bytes UTF-8
    const expectedLen = new TextEncoder().encode(payload).byteLength
    const buf = encodeChatFrame(ReqId.ID_TEXT_CHAT_MSG_REQ, payload)
    expect(new DataView(buf).getUint16(2, false)).toBe(expectedLen)
  })
})

describe("ChatFrameParser", () => {
  const parser = new ChatFrameParser()

  beforeEach(() => parser.reset())

  it("parses a single complete frame", () => {
    const buf = encodeChatFrame(ReqId.ID_CHAT_LOGIN_RSP, '{"error":0}')
    const frames = parser.append(buf)
    expect(frames).toHaveLength(1)
    expect(frames[0]?.reqId).toBe(ReqId.ID_CHAT_LOGIN_RSP)
    expect(frames[0]?.payload).toBe('{"error":0}')
  })

  it("parses two back-to-back frames in one buffer", () => {
    const f1 = encodeChatFrame(ReqId.ID_HEART_BEAT_REQ, "{}")
    const f2 = encodeChatFrame(ReqId.ID_CHAT_LOGIN_RSP, '{"error":0}')
    const merged = new Uint8Array(f1.byteLength + f2.byteLength)
    merged.set(new Uint8Array(f1), 0)
    merged.set(new Uint8Array(f2), f1.byteLength)
    const frames = parser.append(merged.buffer)
    expect(frames).toHaveLength(2)
    expect(frames[0]?.reqId).toBe(ReqId.ID_HEART_BEAT_REQ)
    expect(frames[1]?.reqId).toBe(ReqId.ID_CHAT_LOGIN_RSP)
  })

  it("handles split header across two appends", () => {
    const buf = encodeChatFrame(ReqId.ID_HEARTBEAT_RSP, "{}")
    // Split at byte 2 — right in the middle of the 4-byte header
    const part1 = buf.slice(0, 2)
    const part2 = buf.slice(2)
    const r1 = parser.append(part1)
    expect(r1).toHaveLength(0) // not enough header yet
    const r2 = parser.append(part2)
    expect(r2).toHaveLength(1)
    expect(r2[0]?.reqId).toBe(ReqId.ID_HEARTBEAT_RSP)
  })

  it("handles split payload across two appends", () => {
    const payload = '{"uid":999}'
    const buf = encodeChatFrame(ReqId.ID_TEXT_CHAT_MSG_RSP, payload)
    // Give header + partial payload in first chunk
    const split = 4 + 3
    const r1 = parser.append(buf.slice(0, split))
    expect(r1).toHaveLength(0)
    const r2 = parser.append(buf.slice(split))
    expect(r2).toHaveLength(1)
    expect(r2[0]?.payload).toBe(payload)
  })

  it("handles empty buffer append gracefully", () => {
    const frames = parser.append(new ArrayBuffer(0))
    expect(frames).toHaveLength(0)
  })

  it("reset clears partial state", () => {
    const buf = encodeChatFrame(ReqId.ID_HEART_BEAT_REQ, "{}")
    parser.append(buf.slice(0, 2)) // partial
    parser.reset()
    const frames = parser.append(encodeChatFrame(ReqId.ID_HEARTBEAT_RSP, "{}"))
    expect(frames).toHaveLength(1)
    expect(frames[0]?.reqId).toBe(ReqId.ID_HEARTBEAT_RSP)
  })
})
