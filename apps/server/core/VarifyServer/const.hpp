#pragma once

// VarifyServer-local error codes. Mirrors the shared ErrorCodes enum values used
// across MemoChat services (see GateServer/core/config/const.h and
// ChatServer/infrastructure/const.h). VarifyServer only needs EmailSendFailed,
// but the full enum is kept value-compatible so error codes stay consistent on
// the wire. Defining it locally lets VarifyServer stop pulling the heavyweight
// gate-core const.h (which drags in boost/beast/asio/pqxx) and completes the
// Phase 1 severing of VarifyServer -> GateServerCore.

enum ErrorCodes
{
    Success = 0,
    Error_Json = 1001,
    RPCFailed = 1002,
    VarifyExpired = 1003,
    VarifyCodeErr = 1004,
    UserExist = 1005,
    PasswdErr = 1006,
    EmailNotMatch = 1007,
    PasswdUpFailed = 1008,
    PasswdInvalid = 1009,
    TokenInvalid = 1010,
    UidInvalid = 1011,
    ProfileUpFailed = 1012,
    MediaUploadFailed = 1013,
    ClientVersionTooLow = 1014,
    CallBusy = 1015,
    CallNotFound = 1016,
    CallStateInvalid = 1017,
    CallPermissionDenied = 1018,
    CallTargetOffline = 1019,
    ChatTicketInvalid = 1095,
    ChatTicketExpired = 1096,
    ChatServerMismatch = 1097,
    RateLimited = 1020,
    InvalidEmail = 1021,
    EmailSendFailed = 1022,
};
