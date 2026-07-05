/**
 * ChatMessageDispatcher — mirrors ChatMessageDispatcher.h
 *
 * Domain-agnostic: holds a ReqId→handler map and routes incoming frames.
 * Domain wiring happens in app/dispatch/registerChatRoutes.ts, NOT here.
 */
import type { ReqId } from "@/core/network/opcodes/reqIds"
import type { FrameHandler, IncomingFrame, Unsubscribe } from "./dispatcher.types"

export class ChatMessageDispatcher {
  private readonly _handlers = new Map<ReqId, Set<FrameHandler>>()

  /**
   * Register a handler for a reqId.
   * Returns an unsubscribe function — always call it on cleanup.
   */
  subscribe(reqId: ReqId, handler: FrameHandler): Unsubscribe {
    let set = this._handlers.get(reqId)
    if (!set) {
      set = new Set()
      this._handlers.set(reqId, set)
    }
    set.add(handler)
    return () => {
      this._handlers.get(reqId)?.delete(handler)
    }
  }

  /**
   * Dispatch a decoded frame to all registered handlers.
   * Unknown reqIds are silently dropped (mirrors C++ no-op).
   */
  dispatch(frame: IncomingFrame): void {
    const set = this._handlers.get(frame.reqId)
    if (!set) return
    for (const handler of set) {
      try {
        handler(frame)
      } catch (err) {
        console.error("[Dispatcher] handler threw for reqId", frame.reqId, err)
      }
    }
  }

  /** Remove all registered handlers (call on logout/teardown) */
  clear(): void {
    this._handlers.clear()
  }
}
