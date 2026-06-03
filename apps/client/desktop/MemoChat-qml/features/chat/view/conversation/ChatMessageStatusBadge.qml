import QtQuick 2.15

Text {
    id: root

    property bool outgoing: false
    property string messageState: "sent"

    readonly property bool active: (root.outgoing && root.messageState !== "sent")
                                   || (!root.outgoing && (root.messageState === "edited" || root.messageState === "deleted"))
    readonly property string statusText: {
        if (root.messageState === "sending") {
            return "发送中..."
        }
        if (root.messageState === "failed") {
            return "发送失败"
        }
        if (root.messageState === "accepted") {
            return "已受理"
        }
        if (root.messageState === "queued_retry") {
            return "排队重试"
        }
        if (root.messageState === "offline_pending") {
            return "离线待补投"
        }
        if (root.messageState === "read") {
            return "已读"
        }
        if (root.messageState === "edited") {
            return "已编辑"
        }
        if (root.messageState === "deleted") {
            return "已撤回"
        }
        return ""
    }
    readonly property color statusColor: {
        if (root.messageState === "failed") {
            return "#c74747"
        }
        if (root.messageState === "queued_retry" || root.messageState === "offline_pending") {
            return "#856404"
        }
        return "#6c7d92"
    }

    visible: root.active
    text: root.statusText
    color: root.statusColor
    font.pixelSize: 11
}
