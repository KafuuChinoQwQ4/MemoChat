import type { ReqId } from "@/core/network/opcodes/reqIds"

/** A decoded frame delivered to a handler */
export interface IncomingFrame {
  readonly reqId: ReqId
  readonly length: number
  /** UTF-8 decoded JSON string — call JSON.parse yourself */
  readonly payload: string
}

/** Handler function registered for a specific reqId */
export type FrameHandler = (frame: IncomingFrame) => void

/** Return value of subscribe() — call to remove the handler */
export type Unsubscribe = () => void
