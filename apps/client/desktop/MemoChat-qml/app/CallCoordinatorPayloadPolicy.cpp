#include "CallCoordinatorPayloadPolicy.h"

namespace memochat::call_payload
{
QString mapCallError(int errorCode)
{
    switch (errorCode)
    {
        case 1015:
            return QStringLiteral("当前线路忙，请稍后重试");
        case 1016:
            return QStringLiteral("通话不存在或已失效");
        case 1017:
            return QStringLiteral("当前通话状态不可执行该操作");
        case 1018:
            return QStringLiteral("无权执行该通话操作");
        case 1019:
            return QStringLiteral("对方当前不在线");
        default:
            return QStringLiteral("通话服务失败");
    }
}

QString callStateText(const QString& eventType)
{
    if (eventType == QStringLiteral("call.invite"))
    {
        return QStringLiteral("来电中");
    }
    if (eventType == QStringLiteral("call.accepted"))
    {
        return QStringLiteral("已接通");
    }
    if (eventType == QStringLiteral("call.rejected"))
    {
        return QStringLiteral("对方已拒绝");
    }
    if (eventType == QStringLiteral("call.cancel"))
    {
        return QStringLiteral("对方已取消");
    }
    if (eventType == QStringLiteral("call.timeout"))
    {
        return QStringLiteral("呼叫超时");
    }
    if (eventType == QStringLiteral("call.hangup"))
    {
        return QStringLiteral("通话结束");
    }
    return QStringLiteral("通话处理中");
}

QJsonObject buildLivekitMetadata(const QString& callId,
                                 const QString& roomName,
                                 const QString& callType,
                                 const QString& peerName,
                                 const QString& peerIcon,
                                 bool muted,
                                 bool cameraEnabled,
                                 const QString& role,
                                 const QString& traceId)
{
    QJsonObject metadata;
    metadata["callId"] = callId;
    metadata["roomName"] = roomName;
    metadata["callType"] = callType;
    metadata["peerName"] = peerName;
    metadata["peerIcon"] = peerIcon;
    metadata["muted"] = muted;
    metadata["cameraEnabled"] = cameraEnabled;
    metadata["role"] = role;
    metadata["traceId"] = traceId;
    return metadata;
}

QJsonObject buildLivekitLaunchPayload(const QString& livekitUrl, const QString& token, const QJsonObject& metadata)
{
    QJsonObject launchPayload;
    launchPayload["wsUrl"] = livekitUrl;
    launchPayload["token"] = token;
    launchPayload["metadata"] = metadata;
    return launchPayload;
}

CallEventPeer resolveCallEventPeer(const QJsonObject& payload, int selfUid)
{
    const bool isCaller = payload.value("caller_uid").toInt() == selfUid;
    return {
        isCaller ? payload.value("callee_name").toString() : payload.value("caller_name").toString(),
        isCaller ? payload.value("callee_icon").toString() : payload.value("caller_icon").toString(),
        isCaller,
    };
}
} // namespace memochat::call_payload
