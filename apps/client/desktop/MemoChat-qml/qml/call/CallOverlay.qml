import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebEngine 1.15
import "../components"

Item {
    id: root
    property var sessionModel
    property string lastLaunchJson: ""
    signal acceptRequested()
    signal rejectRequested()
    signal endRequested()
    signal muteToggled()
    signal cameraToggled()

    anchors.fill: parent
    visible: sessionModel && sessionModel.visible

    readonly property bool incomingState: sessionModel && sessionModel.incoming
    readonly property bool mediaState: sessionModel && sessionModel.active && sessionModel.tokenReady

    function runBridgeScript(source) {
        if (!mediaView || !source || !root.mediaState) {
            return
        }
        mediaView.runJavaScript(source)
    }

    function launchMediaIfReady() {
        if (!root.mediaState || !sessionModel || !sessionModel.mediaLaunchJson) {
            return
        }
        if (lastLaunchJson === sessionModel.mediaLaunchJson) {
            return
        }
        lastLaunchJson = sessionModel.mediaLaunchJson
        runBridgeScript("window.MemoChatLiveKit && window.MemoChatLiveKit.join(" + sessionModel.mediaLaunchJson + ");")
    }

    function forwardBridgeEvent(encodedTitle) {
        const prefix = "__memochat__:"
        if (!encodedTitle || !encodedTitle.startsWith(prefix)) {
            return
        }
        const payloadText = encodedTitle.substring(prefix.length)
        let payload = {}
        try {
            payload = JSON.parse(payloadText)
        } catch (error) {
            livekitBridge.reportMediaError("LiveKit 页面事件解析失败")
            return
        }

        const type = payload.type || ""
        if (type === "joined") {
            livekitBridge.reportRoomJoined()
        } else if (type === "remote_track_ready") {
            livekitBridge.reportRemoteTrackReady()
        } else if (type === "disconnected") {
            livekitBridge.reportRoomDisconnected(payload.reason || "", payload.recoverable === true)
        } else if (type === "permission_error") {
            livekitBridge.reportPermissionError(payload.deviceType || "", payload.message || "")
        } else if (type === "media_error") {
            livekitBridge.reportMediaError(payload.message || "")
        } else if (type === "reconnecting") {
            livekitBridge.reportReconnecting(payload.message || "")
        } else if (type === "log") {
            livekitBridge.reportLog(payload.message || "")
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.03, 0.06, 0.10, 0.82)
    }

    WebEngineView {
        id: mediaView
        anchors.fill: parent
        visible: root.mediaState && livekitBridge && livekitBridge.embeddedEnabled
        url: livekitBridge ? livekitBridge.roomPageUrl : ""
        settings.javascriptEnabled: true
        settings.localContentCanAccessRemoteUrls: true
        settings.localContentCanAccessFileUrls: true
        settings.playbackRequiresUserGesture: false

        onLoadingChanged: function(loadRequest) {
            if (loadRequest.status === WebEngineLoadRequest.LoadSucceededStatus) {
                root.launchMediaIfReady()
            } else if (loadRequest.status === WebEngineLoadRequest.LoadFailedStatus) {
                livekitBridge.reportMediaError("LiveKit 页面加载失败")
            }
        }

        onTitleChanged: root.forwardBridgeEvent(title)

        onFeaturePermissionRequested: function(securityOrigin, feature) {
            grantFeaturePermission(securityOrigin, feature, true)
        }
    }

    Connections {
        target: sessionModel
        function onChanged() {
            if (!root.mediaState) {
                root.lastLaunchJson = ""
                return
            }
            root.launchMediaIfReady()
        }
    }

    Connections {
        target: livekitBridge
        function onJoinRequested() {
            root.lastLaunchJson = ""
            root.launchMediaIfReady()
        }
        function onMicToggleRequested() {
            root.runBridgeScript("window.MemoChatLiveKit && window.MemoChatLiveKit.toggleMic();")
        }
        function onCameraToggleRequested() {
            root.runBridgeScript("window.MemoChatLiveKit && window.MemoChatLiveKit.toggleCamera();")
        }
        function onLeaveRequested() {
            root.runBridgeScript("window.MemoChatLiveKit && window.MemoChatLiveKit.leave();")
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.mediaState
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.06)
    }

    GlassSurface {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: root.mediaState ? 18 : (parent.height - height) / 2
        width: root.mediaState ? Math.min(parent.width - 32, 860) : Math.min(parent.width - 40, 540)
        height: root.mediaState ? 170 : Math.min(parent.height - 60, 430)
        cornerRadius: 24
        blurRadius: 24
        fillColor: root.mediaState ? Qt.rgba(0.08, 0.12, 0.18, 0.72) : Qt.rgba(0.10, 0.16, 0.22, 0.90)
        strokeColor: Qt.rgba(1, 1, 1, 0.18)
        glowTopColor: Qt.rgba(1, 1, 1, 0.10)
        glowBottomColor: Qt.rgba(0.12, 0.44, 0.72, 0.12)

        Behavior on anchors.topMargin {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutQuad
            }
        }

        Behavior on width {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutQuad
            }
        }

        Behavior on height {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutQuad
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: root.mediaState ? 20 : 28
            spacing: root.mediaState ? 12 : 18

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                Rectangle {
                    width: root.mediaState ? 64 : 92
                    height: root.mediaState ? 64 : 92
                    radius: width / 2
                    color: Qt.rgba(1, 1, 1, 0.08)
                    border.color: Qt.rgba(1, 1, 1, 0.18)

                    Image {
                        anchors.fill: parent
                        anchors.margins: 6
                        fillMode: Image.PreserveAspectCrop
                        source: sessionModel ? sessionModel.peerIcon : ""
                        smooth: true
                        clip: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        Layout.fillWidth: true
                        text: sessionModel ? sessionModel.peerName : ""
                        color: "white"
                        font.pixelSize: root.mediaState ? 22 : 28
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: sessionModel
                              ? (sessionModel.callType === "video" ? "视频通话" : "语音通话")
                              : ""
                        color: Qt.rgba(0.84, 0.91, 0.97, 0.82)
                        font.pixelSize: 14
                    }

                    Text {
                        Layout.fillWidth: true
                        text: sessionModel ? sessionModel.stateText : ""
                        color: Qt.rgba(0.61, 0.84, 0.98, 0.96)
                        font.pixelSize: 16
                    }

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: sessionModel ? sessionModel.mediaStatusText : ""
                        color: Qt.rgba(0.86, 0.89, 0.93, 0.76)
                        font.pixelSize: 13
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: sessionModel && sessionModel.active
                        text: sessionModel ? ("持续 " + sessionModel.elapsedSeconds + " 秒") : ""
                        color: Qt.rgba(0.86, 0.89, 0.93, 0.68)
                        font.pixelSize: 13
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                visible: !root.mediaState
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 14
                visible: sessionModel && sessionModel.active

                Button {
                    text: sessionModel && sessionModel.muted ? "取消静音" : "静音"
                    enabled: root.mediaState
                    onClicked: root.muteToggled()
                }

                Button {
                    visible: sessionModel && sessionModel.callType === "video"
                    text: sessionModel && sessionModel.cameraEnabled ? "关闭摄像头" : "打开摄像头"
                    enabled: root.mediaState
                    onClicked: root.cameraToggled()
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 16

                Button {
                    visible: root.incomingState
                    text: "拒绝"
                    onClicked: root.rejectRequested()
                }

                Button {
                    visible: root.incomingState
                    text: "接听"
                    onClicked: root.acceptRequested()
                }

                Button {
                    visible: sessionModel && !sessionModel.incoming
                    text: sessionModel && sessionModel.active ? "挂断" : "取消"
                    onClicked: root.endRequested()
                }
            }
        }
    }
}
