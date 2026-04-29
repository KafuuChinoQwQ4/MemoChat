import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var agentController: null
    property var messageModel: null
    property bool loading: false
    property bool streaming: false
    property string errorMsg: ""
    property var sessions: []
    property string currentSessionId: ""
    property string selfAvatar: "qrc:/res/head_1.jpg"
    property bool stickToBottom: true

    signal reloadSessions()
    signal switchSession(string sessionId)
    signal deleteSession(string sessionId)

    function isNearBottom() {
        return messageListView.contentY + messageListView.height >= messageListView.contentHeight - 36
    }

    function scrollToEndSoon() {
        Qt.callLater(function() {
            if (messageListView.count > 0) {
                messageListView.forceLayout()
                messageListView.positionViewAtEnd()
            }
        })
    }

    Connections {
        target: root.messageModel
        ignoreUnknownSignals: true

        function onDataChanged(topLeft, bottomRight, roles) {
            if (root.stickToBottom || root.streaming) {
                root.scrollToEndSoon()
            }
        }

        function onModelReset() {
            root.stickToBottom = true
            root.scrollToEndSoon()
        }
    }

    ListView {
        id: messageListView
        anchors.fill: parent
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        clip: true
        model: root.messageModel
        spacing: 4
        ScrollBar.vertical: GlassScrollBar { }

        delegate: AgentMessageDelegate {
            width: messageListView.width
            msgId: model.msgId || ""
            content: model.content || ""
            role: model.role || "assistant"
            isUser: model.role === "user"
            isAssistant: model.role === "assistant"
            isStreaming: model.isStreaming || false
            streamingContent: model.streamingContent || model.content || ""
            thinkingContent: model.thinkingContent || ""
            errorMessage: model.errorMessage || ""
            createdAt: model.createdAt || 0
            sourcesJson: model.sources || ""
            selfAvatar: root.selfAvatar
        }

        onCountChanged: {
            root.stickToBottom = true
            root.scrollToEndSoon()
        }

        onContentHeightChanged: {
            if (root.stickToBottom || root.loading || root.streaming) {
                root.scrollToEndSoon()
            }
        }

        onMovementEnded: root.stickToBottom = root.isNearBottom()
        onFlickEnded: root.stickToBottom = root.isNearBottom()
        onContentYChanged: {
            if (!moving && !flicking) {
                root.stickToBottom = root.isNearBottom()
            }
        }
    }

    GlassSurface {
        anchors.centerIn: parent
        width: 260
        height: 104
        visible: root.messageModel && root.messageModel.rowCount() === 0
        backdrop: root
        cornerRadius: 14
        blurRadius: 18
        fillColor: Qt.rgba(1, 1, 1, 0.18)
        strokeColor: Qt.rgba(1, 1, 1, 0.40)

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 6

            Label {
                text: root.currentSessionId.length > 0 ? "开始和 AI 聊天吧" : "从左侧选择或新建会话"
                color: "#2a3649"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                text: "主区现在只保留对话内容，减少干扰。"
                color: "#6a7b92"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        width: Math.min(parent.width - 40, errorText.implicitWidth + 26)
        height: errorText.implicitHeight + 14
        radius: 10
        color: Qt.rgba(0.89, 0.27, 0.27, 0.12)
        border.color: Qt.rgba(0.89, 0.27, 0.27, 0.24)
        visible: root.errorMsg.length > 0

        Label {
            id: errorText
            anchors.centerIn: parent
            text: root.errorMsg
            color: "#c14d4d"
            font.pixelSize: 12
        }
    }
}
