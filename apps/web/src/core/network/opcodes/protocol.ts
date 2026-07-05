/** Protocol version constants — keep in sync with global.h */
export const PROTOCOL_VERSION   = 3
export const CLIENT_VERSION     = "3.0.0"
export const CHAT_COUNT_PER_PAGE = 13

/** Chat frame header size: 2 bytes msgId + 2 bytes len */
export const CHAT_FRAME_HEADER_LEN = 4

/** Heartbeat interval (ms) — send ID_HEART_BEAT_REQ every N ms */
export const HEARTBEAT_INTERVAL_MS = 25_000

/** Max missed heartbeat acks before triggering reconnect */
export const HEARTBEAT_MAX_MISS = 2

/** Reconnect backoff: base delay in ms */
export const RECONNECT_BASE_DELAY_MS = 1_500

/** Max uint16 payload length */
export const MAX_FRAME_PAYLOAD = 65_535
