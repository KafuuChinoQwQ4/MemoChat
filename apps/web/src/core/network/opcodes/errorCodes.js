/** Error codes from global.h + Envoy gateway */
export var ErrorCodes;
(function (ErrorCodes) {
    ErrorCodes[ErrorCodes["SUCCESS"] = 0] = "SUCCESS";
    ErrorCodes[ErrorCodes["ERR_JSON"] = 1] = "ERR_JSON";
    ErrorCodes[ErrorCodes["ERR_NETWORK"] = 2] = "ERR_NETWORK";
    ErrorCodes[ErrorCodes["ERR_VERSION_TOO_LOW"] = 1014] = "ERR_VERSION_TOO_LOW";
    ErrorCodes[ErrorCodes["ERR_CHAT_TICKET_INVALID"] = 1095] = "ERR_CHAT_TICKET_INVALID";
    ErrorCodes[ErrorCodes["ERR_CHAT_TICKET_EXPIRED"] = 1096] = "ERR_CHAT_TICKET_EXPIRED";
    ErrorCodes[ErrorCodes["ERR_CHAT_SERVER_MISMATCH"] = 1097] = "ERR_CHAT_SERVER_MISMATCH";
    ErrorCodes[ErrorCodes["ERR_PROTOCOL_VERSION"] = 1098] = "ERR_PROTOCOL_VERSION";
})(ErrorCodes || (ErrorCodes = {}));
/** HTTP status codes that require special handling */
export const HTTP_RATE_LIMITED = 429; // x-envoy-ratelimited
export const HTTP_HOST_NOT_ALLOWED = 421; // wrong virtual host
