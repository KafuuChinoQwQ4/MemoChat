/** Generate a client-side message ID */
let _seq = 0
export function genClientMsgId(): string {
  return `c_${Date.now()}_${_seq++}`
}
