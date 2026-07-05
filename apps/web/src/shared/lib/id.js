/** Generate a client-side message ID */
let _seq = 0;
export function genClientMsgId() {
    return `c_${Date.now()}_${_seq++}`;
}
