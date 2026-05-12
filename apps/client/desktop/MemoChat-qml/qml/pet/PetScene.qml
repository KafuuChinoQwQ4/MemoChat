import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0

Item {
    id: root
    property var petController: null
    property bool alwaysOnTop: true
    property bool clickThrough: false
    property bool decorativeMode: false
    property bool debugPanelVisible: false
    property real scaleFactor: 1.0
    property bool micMuted: true
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false
    property bool localOnlyMode: true
    property bool debugRetentionEnabled: false
    property bool providerAvailable: false
    signal closeRequested()
    signal resetPositionRequested()
    signal alwaysOnTopToggled(bool value)
    signal clickThroughToggled(bool value)
    signal debugToggled(bool value)
    signal scaleRequested(real value)
    signal micMuteToggled(bool value)
    signal cameraToggled(bool value)
    signal cloudVisionToggled(bool value)
    signal localOnlyModeToggled(bool value)
    signal debugRetentionToggled(bool value)

    function phaseText(phase) {
        switch (phase) {
        case "listening":
            return "倾听"
        case "thinking":
            return "思考"
        case "speaking":
            return "回应"
        case "interrupted":
            return "已打断"
        case "error":
            return "异常"
        case "idle":
            return "空闲"
        default:
            return phase || ""
        }
    }

    function phaseColor(phase) {
        switch (phase) {
        case "listening":
            return "#67d5c1"
        case "thinking":
            return "#f1c76f"
        case "speaking":
            return "#7db5ff"
        case "interrupted":
            return "#d8a46f"
        case "error":
            return "#e35b5b"
        default:
            return root.petController && root.petController.streaming ? "#42b883" : "#d4a23f"
        }
    }

    function displayStatus() {
        if (!root.petController) {
            return "桌宠"
        }
        var phase = phaseText(root.petController.phase)
        var status = root.petController.statusText || ""
        if (root.petController.error.length > 0) {
            status = root.petController.error
        }
        if (status.length > 0 && phase.length > 0) {
            return status + " · " + phase
        }
        return status.length > 0 ? status : (phase.length > 0 ? phase : "桌宠")
    }

    function privacyColor(active, warning) {
        if (warning) {
            return "#f1c76f"
        }
        return active ? "#67d5c1" : "#9aa7b8"
    }

    function micPrivacyText() {
        return root.micMuted ? "麦克风 静音" : "麦克风 本地输入"
    }

    function cameraPrivacyText() {
        return root.cameraEnabled ? "摄像头 本地结构信号" : "摄像头 关闭"
    }

    function cloudPrivacyText() {
        if (!root.cloudVisionEnabled) {
            return "云视觉 关闭"
        }
        return root.providerAvailable ? "云视觉 已授权" : "云视觉 无提供方"
    }

    function localModeText() {
        return root.localOnlyMode ? "本地优先" : "允许云能力"
    }

    function debugRetentionText() {
        return root.debugRetentionEnabled ? "调试保留 开启" : "调试保留 关闭"
    }

    Live2DRenderItem {
        id: live2d
        anchors.fill: parent
        anchors.margins: root.decorativeMode ? 4 : 8
        expression: root.petController ? root.petController.expression : "neutral"
        motion: root.petController ? root.petController.motion : "idle"
        emotion: root.petController ? root.petController.emotion : "neutral"
        intensity: root.petController ? root.petController.intensity : 0.35
        gazeX: root.petController ? root.petController.gazeX : 0.5
        gazeY: root.petController ? root.petController.gazeY : 0.5
        lipSyncValue: root.petController ? root.petController.lipSyncValue : 0
    }

    Connections {
        target: root.petController
        function onControlEventReceived(event) {
            live2d.applyControlEvent(event)
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: composer.top
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        anchors.bottomMargin: 8
        height: Math.min(58, speechLabel.implicitHeight + 18)
        radius: 8
        color: Qt.rgba(1, 1, 1, 0.68)
        border.color: Qt.rgba(1, 1, 1, 0.75)
        visible: !root.decorativeMode && root.petController && root.petController.speechText.length > 0

        Label {
            id: speechLabel
            anchors.fill: parent
            anchors.margins: 9
            text: root.petController ? root.petController.speechText : ""
            color: "#253142"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }
    }

    RowLayout {
        id: topControls
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        height: 28
        spacing: 6

        Rectangle {
            Layout.preferredWidth: 9
            Layout.preferredHeight: 9
            radius: 5
            color: root.petController && root.petController.error.length > 0
                   ? "#e35b5b"
                   : root.phaseColor(root.petController ? root.petController.phase : "")
        }

        Label {
            Layout.fillWidth: true
            text: root.displayStatus()
            color: "#f7fbff"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        ToolButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            text: root.alwaysOnTop ? "P" : "p"
            ToolTip.visible: hovered
            ToolTip.text: "置顶"
            onClicked: root.alwaysOnTopToggled(!root.alwaysOnTop)
        }

        ToolButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            text: root.clickThrough ? "I" : "i"
            ToolTip.visible: hovered
            ToolTip.text: "点击穿透"
            onClicked: root.clickThroughToggled(!root.clickThrough)
        }

        ToolButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            text: root.debugPanelVisible ? "D" : "d"
            ToolTip.visible: hovered
            ToolTip.text: "调试"
            onClicked: root.debugToggled(!root.debugPanelVisible)
        }

        ToolButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            text: "R"
            ToolTip.visible: hovered
            ToolTip.text: "复位"
            onClicked: root.resetPositionRequested()
        }

        ToolButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            text: "x"
            onClicked: root.closeRequested()
        }
    }

    GridLayout {
        id: privacyChips
        anchors.top: topControls.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.topMargin: 4
        columns: 2
        rowSpacing: 4
        columnSpacing: 6
        visible: !root.decorativeMode

        StatusChip {
            Layout.fillWidth: true
            text: root.micPrivacyText()
            colorBase: root.privacyColor(!root.micMuted, false)
        }

        StatusChip {
            Layout.fillWidth: true
            text: root.cameraPrivacyText()
            colorBase: root.privacyColor(root.cameraEnabled, false)
        }

        StatusChip {
            Layout.fillWidth: true
            text: root.cloudPrivacyText()
            colorBase: root.privacyColor(root.cloudVisionEnabled && root.providerAvailable,
                                         root.cloudVisionEnabled && !root.providerAvailable)
        }

        StatusChip {
            Layout.fillWidth: true
            text: root.localModeText()
            colorBase: root.privacyColor(root.localOnlyMode, false)
        }
    }

    Rectangle {
        id: debugPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: privacyChips.bottom
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.topMargin: 6
        height: debugColumn.implicitHeight + 14
        radius: 8
        color: Qt.rgba(0.08, 0.10, 0.14, 0.70)
        border.color: Qt.rgba(1, 1, 1, 0.18)
        visible: root.debugPanelVisible && !root.decorativeMode

        ColumnLayout {
            id: debugColumn
            anchors.fill: parent
            anchors.margins: 7
            spacing: 4

            Label {
                Layout.fillWidth: true
                text: "seq " + (root.petController ? root.petController.model.sequence : 0)
                      + " · phase " + (root.petController ? root.petController.phase : "none")
                      + " · scale " + root.scaleFactor.toFixed(2)
                color: "#edf5ff"
                font.pixelSize: 10
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: "event " + (root.petController ? root.petController.eventId : "")
                      + " · turn " + (root.petController ? root.petController.turnId : "")
                color: "#b7c7dc"
                font.pixelSize: 10
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: root.debugRetentionText()
                      + " · provider " + (root.providerAvailable ? "ready" : "unavailable")
                color: root.debugRetentionEnabled ? "#f1c76f" : "#b7c7dc"
                font.pixelSize: 10
                elide: Text.ElideRight
            }
        }
    }

    RowLayout {
        id: composer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.bottomMargin: 14
        height: 34
        spacing: 8
        visible: !root.decorativeMode && !root.clickThrough
        enabled: !root.decorativeMode && !root.clickThrough

        TextField {
            id: input
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            placeholderText: "对桌宠说点什么"
            font.pixelSize: 12
            selectByMouse: true
            background: Rectangle {
                radius: 8
                color: Qt.rgba(255, 255, 255, 0.74)
                border.color: input.activeFocus ? "#74b2ba" : Qt.rgba(255, 255, 255, 0.76)
            }
            onAccepted: sendButton.clicked()
        }

        Button {
            id: sendButton
            Layout.preferredWidth: 54
            Layout.preferredHeight: 34
            text: "发送"
            enabled: root.petController && input.text.trim().length > 0
            onClicked: {
                if (!root.petController) {
                    return
                }
                root.petController.sendText(input.text)
                input.text = ""
            }
        }
    }

    Rectangle {
        id: dockControls
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: composer.visible ? composer.top : parent.bottom
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.bottomMargin: composer.visible ? 8 : 14
        height: controlsRow.implicitHeight + 12
        radius: 8
        color: Qt.rgba(0.06, 0.08, 0.11, 0.46)
        border.color: Qt.rgba(1, 1, 1, 0.16)
        visible: !root.decorativeMode

        RowLayout {
            id: controlsRow
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 7

            ToolButton {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 28
                text: root.micMuted ? "M" : "m"
                ToolTip.visible: hovered
                ToolTip.text: "麦克风"
                onClicked: root.micMuteToggled(!root.micMuted)
            }

            ToolButton {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 28
                text: root.cameraEnabled ? "C" : "c"
                ToolTip.visible: hovered
                ToolTip.text: "摄像头"
                onClicked: root.cameraToggled(!root.cameraEnabled)
            }

            ToolButton {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 28
                text: root.cloudVisionEnabled ? "V" : "v"
                ToolTip.visible: hovered
                ToolTip.text: "云视觉"
                onClicked: root.cloudVisionToggled(!root.cloudVisionEnabled)
            }

            ToolButton {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 28
                text: root.localOnlyMode ? "L" : "l"
                ToolTip.visible: hovered
                ToolTip.text: "本地模式"
                onClicked: root.localOnlyModeToggled(!root.localOnlyMode)
            }

            ToolButton {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 28
                visible: root.debugPanelVisible && !root.decorativeMode
                text: root.debugRetentionEnabled ? "T" : "t"
                ToolTip.visible: hovered
                ToolTip.text: "调试保留"
                onClicked: root.debugRetentionToggled(!root.debugRetentionEnabled)
            }

            Label {
                text: Math.round(root.scaleFactor * 100) + "%"
                color: "#edf5ff"
                font.pixelSize: 11
            }

            Slider {
                id: scaleSlider
                Layout.fillWidth: true
                from: 0.65
                to: 1.45
                value: root.scaleFactor
                onMoved: root.scaleRequested(value)
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topControls.bottom
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.topMargin: 2
        height: errorLabel.implicitHeight + 12
        radius: 8
        color: Qt.rgba(0.88, 0.22, 0.22, 0.72)
        visible: root.petController && root.petController.error.length > 0

        Label {
            id: errorLabel
            anchors.fill: parent
            anchors.margins: 6
            text: root.petController ? root.petController.error : ""
            color: "white"
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }

    component StatusChip: Rectangle {
        property string text: ""
        property color colorBase: "#7db5ff"
        implicitWidth: chipText.implicitWidth + 14
        implicitHeight: 22
        radius: 8
        color: Qt.rgba(colorBase.r, colorBase.g, colorBase.b, 0.22)
        border.color: Qt.rgba(colorBase.r, colorBase.g, colorBase.b, 0.34)

        Label {
            id: chipText
            anchors.fill: parent
            anchors.leftMargin: 7
            anchors.rightMargin: 7
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: parent.text
            color: "#f7fbff"
            font.pixelSize: 10
            font.bold: true
            elide: Text.ElideRight
        }
    }
}
