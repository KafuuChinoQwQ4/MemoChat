pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

Window {
    id: root
    width: 320
    height: 560
    minimumWidth: 300
    minimumHeight: 360
    maximumWidth: 320
    title: "桌宠控制"
    flags: (Qt.platform.os === "linux" ? Qt.Window : Qt.Tool)
           | Qt.FramelessWindowHint
           | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: false

    property var petController: null
    property var agentController: null
    property bool alwaysOnTop: true
    property bool clickThrough: false
    property bool debugPanelVisible: false
    property real scaleFactor: 1.0
    property bool micMuted: true
    property bool cameraEnabled: false
    property bool cloudVisionEnabled: false
    property bool localOnlyMode: true
    property bool debugRetentionEnabled: false
    property bool voiceReplyEnabled: true
    property bool providerAvailable: false
    property string cameraCaptureStatus: cameraEnabled ? "摄像头本地捕捉" : "摄像头关闭"

    readonly property color accentColor: "#74b2ba"
    readonly property color panelColor: "#fffafd"
    readonly property color borderColor: "#ead6e1"
    readonly property var availableModels: agentController ? agentController.availableModels : []
    readonly property string currentModel: agentController ? agentController.currentModel : ""
    readonly property bool apiProviderBusy: agentController ? agentController.apiProviderBusy : false
    readonly property string apiProviderStatus: agentController ? agentController.apiProviderStatus : ""
    readonly property bool modelRefreshBusy: agentController ? agentController.modelRefreshBusy : false

    signal closePetRequested()
    signal chatRequested()
    signal resetPositionRequested()
    signal alwaysOnTopToggled(bool value)
    signal clickThroughToggled(bool value)
    signal debugToggled(bool value)
    signal scaleInteractionStarted()
    signal scaleRequested(real value)
    signal scaleInteractionFinished()
    signal micMuteToggled(bool value)
    signal cameraToggled(bool value)
    signal cloudVisionToggled(bool value)
    signal localOnlyModeToggled(bool value)
    signal debugRetentionToggled(bool value)
    signal voiceReplyToggled(bool value)

    function openPanel() {
        show()
        raise()
        requestActivate()
        if (root.agentController) {
            root.agentController.refreshModelList()
        }
    }

    function displayStatus() {
        if (!root.petController) {
            return "桌宠"
        }
        var phase = root.petController.phase || ""
        var status = root.petController.statusText || ""
        if (root.petController.error.length > 0) {
            status = root.petController.error
        }
        return status.length > 0 ? status : (phase.length > 0 ? phase : "桌宠")
    }

    function sendQuickText(text) {
        if (!root.petController || text.length === 0) {
            return
        }
        root.petController.sendText(text)
    }

    function registerApiProvider() {
        if (!root.agentController || root.apiProviderBusy) {
            return
        }
        root.agentController.registerApiProvider(apiProviderNameField.text,
                                                 apiBaseUrlField.text,
                                                 apiKeyField.text)
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 6
        radius: 14
        antialiasing: true
        color: Qt.rgba(1.0, 0.98, 0.995, 0.96)
        border.color: root.borderColor

        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            radius: parent.radius
            antialiasing: true
            color: Qt.rgba(0.37, 0.25, 0.34, 0.14)
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 9
                    Layout.preferredHeight: 9
                    radius: 5
                    color: root.petController && root.petController.error.length > 0 ? "#e35b5b" : "#74b2ba"
                }

                Label {
                    Layout.fillWidth: true
                    text: root.displayStatus()
                    color: "#4b3042"
                    font.pixelSize: 13
                    font.bold: true
                    elide: Text.ElideRight
                }

                Button {
                    id: panelCloseButton
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    text: "×"
                    padding: 0
                    onClicked: root.hide()
                    background: Rectangle {
                        radius: 14
                        antialiasing: true
                        color: panelCloseButton.down ? "#f1d7e3"
                                                     : panelCloseButton.hovered ? "#f8e6ee" : "#fffafd"
                        border.color: "#dcc8d4"
                    }
                    contentItem: Label {
                        text: panelCloseButton.text
                        color: "#6c4a5e"
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Flickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                contentWidth: width
                contentHeight: controlColumn.implicitHeight

                ColumnLayout {
                    id: controlColumn
                    width: parent.width
                    spacing: 10

                    PetMenuButton {
                        Layout.fillWidth: true
                        text: "聊天"
                        enabled: root.petController
                        onClicked: root.chatRequested()
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        rowSpacing: 8
                        columnSpacing: 8

                        PetMenuButton {
                            Layout.fillWidth: true
                            text: "打招呼"
                            enabled: root.petController
                            onClicked: root.sendQuickText("向我打个招呼")
                        }

                        PetMenuButton {
                            Layout.fillWidth: true
                            text: "打断"
                            enabled: root.petController && root.petController.sessionId.length > 0
                            onClicked: root.petController.interrupt()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.borderColor
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "置顶"
                        checked: root.alwaysOnTop
                        onToggled: function(checked) { root.alwaysOnTopToggled(checked) }
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "点击穿透"
                        checked: root.clickThrough
                        onToggled: function(checked) { root.clickThroughToggled(checked) }
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "麦克风"
                        checked: !root.micMuted
                        onToggled: function(checked) { root.micMuteToggled(!checked) }
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "摄像头"
                        checked: root.cameraEnabled
                        onToggled: function(checked) { root.cameraToggled(checked) }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: root.cameraEnabled
                        text: root.cameraCaptureStatus
                        color: "#9fb0c3"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "云视觉"
                        checked: root.cloudVisionEnabled
                        onToggled: function(checked) { root.cloudVisionToggled(checked) }
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "本地优先"
                        checked: root.localOnlyMode
                        onToggled: function(checked) { root.localOnlyModeToggled(checked) }
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "语音回复"
                        checked: root.voiceReplyEnabled
                        onToggled: function(checked) { root.voiceReplyToggled(checked) }
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "调试"
                        checked: root.debugPanelVisible
                        onToggled: function(checked) { root.debugToggled(checked) }
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        visible: root.debugPanelVisible
                        text: "调试保留"
                        checked: root.debugRetentionEnabled
                        onToggled: function(checked) { root.debugRetentionToggled(checked) }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            Layout.fillWidth: true
                            text: "缩放 " + Math.round(scaleSlider.value * 100) + "%"
                            color: "#4b3042"
                            font.pixelSize: 12
                        }

                        Slider {
                            id: scaleSlider
                            Layout.fillWidth: true
                            from: 0.65
                            to: 2.2
                            value: root.scaleFactor
                            live: false
                            onPressedChanged: {
                                if (pressed) {
                                    root.scaleInteractionStarted()
                                } else {
                                    root.scaleRequested(value)
                                    root.scaleInteractionFinished()
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.borderColor
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: "AI API 接入"
                                color: "#4b3042"
                                font.pixelSize: 13
                                font.bold: true
                            }

                            Label {
                                text: root.apiProviderBusy ? "解析中" : ""
                                color: "#6a7b92"
                                font.pixelSize: 11
                                visible: root.apiProviderBusy
                            }
                        }

                        PetTextField {
                            id: apiProviderNameField
                            Layout.fillWidth: true
                            placeholderText: "名称，例如 gpt"
                            text: "gpt"
                        }

                        PetTextField {
                            id: apiBaseUrlField
                            Layout.fillWidth: true
                            placeholderText: "API 地址，例如 https://api.openai.com/v1"
                            text: "https://api.openai.com/v1"
                        }

                        PetTextField {
                            id: apiKeyField
                            Layout.fillWidth: true
                            placeholderText: "API Key"
                            echoMode: TextInput.Password
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: root.apiProviderStatus
                                color: root.apiProviderStatus.indexOf("已接入") >= 0 ? "#4d7f5c" : "#6a7b92"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            PetMenuButton {
                                Layout.preferredWidth: 82
                                text: root.apiProviderBusy ? "解析中" : "接入"
                                enabled: root.agentController && !root.apiProviderBusy
                                onClicked: root.registerApiProvider()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: root.currentModel.length > 0 ? root.currentModel : "未选择模型"
                                color: "#6a7b92"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            PetMenuButton {
                                Layout.preferredWidth: 82
                                text: root.modelRefreshBusy ? "刷新中" : "刷新"
                                enabled: root.agentController && !root.modelRefreshBusy
                                onClicked: root.agentController.refreshModelList()
                            }
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(148, Math.max(64, contentHeight))
                            clip: true
                            model: root.availableModels
                            spacing: 6

                            delegate: Rectangle {
                                id: modelRow
                                required property var modelData

                                width: ListView.view.width
                                height: 42
                                radius: 8
                                antialiasing: true
                                color: {
                                    var fullName = (modelRow.modelData.model_type || "") + ":" + (modelRow.modelData.model_name || "")
                                    if (fullName === root.currentModel) {
                                        return Qt.rgba(0.45, 0.70, 0.73, 0.18)
                                    }
                                    return modelMouse.containsMouse ? "#e8f6f4" : "#fffafd"
                                }
                                border.color: "#ead6e1"

                                MouseArea {
                                    id: modelMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root.agentController && modelRow.modelData.model_type && modelRow.modelData.model_name) {
                                            root.agentController.switchModel(modelRow.modelData.model_type, modelRow.modelData.model_name)
                                        }
                                    }
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 1

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelRow.modelData.display_name || modelRow.modelData.model_name || ""
                                        color: "#4b3042"
                                        font.pixelSize: 12
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        Layout.fillWidth: true
                                        text: modelRow.modelData.model_type || ""
                                        color: "#8f7c88"
                                        font.pixelSize: 10
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        PetMenuButton {
                            Layout.fillWidth: true
                            text: "复位"
                            onClicked: root.resetPositionRequested()
                        }

                        PetMenuButton {
                            Layout.fillWidth: true
                            text: "关闭桌宠"
                            onClicked: root.closePetRequested()
                        }
                    }
                }
            }
        }
    }

    component PetTextField: TextField {
        id: field
        Layout.preferredHeight: 32
        font.pixelSize: 12
        selectByMouse: true
        color: "#2d2630"
        placeholderTextColor: "#8f7c88"
        background: Rectangle {
            radius: 8
            antialiasing: true
            color: "#ffffff"
            border.color: field.activeFocus ? root.accentColor : "#dcc8d4"
        }
    }

    component PetMenuButton: Button {
        id: button
        font.pixelSize: 12
        padding: 8
        background: Rectangle {
            radius: 8
            antialiasing: true
            color: button.down ? "#d6f0ed"
                               : button.hovered ? "#e8f6f4"
                                                : "#fffafd"
            border.color: button.enabled ? "#dcc8d4" : "#eadfe6"
        }
        contentItem: Label {
            text: button.text
            color: button.enabled ? "#4b3042" : "#a997a3"
            font: button.font
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    component OptionSwitch: RowLayout {
        id: option
        property string text: ""
        property alias checked: optionSwitch.checked
        signal toggled(bool checked)
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: option.text
            color: "#4b3042"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Switch {
            id: optionSwitch
            Layout.preferredWidth: 48
            Layout.preferredHeight: 24
            onToggled: option.toggled(optionSwitch.checked)
            indicator: Rectangle {
                implicitWidth: 46
                implicitHeight: 24
                radius: 12
                antialiasing: true
                color: optionSwitch.checked ? root.accentColor : "#eadfe6"
                border.color: optionSwitch.checked ? "#5f9fa7" : "#dcc8d4"

                Rectangle {
                    width: 18
                    height: 18
                    x: optionSwitch.checked ? parent.width - width - 3 : 3
                    y: 3
                    radius: 9
                    antialiasing: true
                    color: "#ffffff"
                }
            }
            contentItem: Item {}
        }
    }
}
