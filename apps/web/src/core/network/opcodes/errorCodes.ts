/** Error codes from global.h + Envoy gateway */
export enum ErrorCodes {
  SUCCESS                 = 0,
  ERR_JSON                = 1,
  ERR_NETWORK             = 2,
  ERR_VERSION_TOO_LOW     = 1014,
  ERR_CHAT_TICKET_INVALID = 1095,
  ERR_CHAT_TICKET_EXPIRED = 1096,
  ERR_CHAT_SERVER_MISMATCH = 1097,
  ERR_PROTOCOL_VERSION    = 1098,
}

/** HTTP status codes that require special handling */
export const HTTP_RATE_LIMITED    = 429   // x-envoy-ratelimited
export const HTTP_HOST_NOT_ALLOWED = 421  // wrong virtual host
