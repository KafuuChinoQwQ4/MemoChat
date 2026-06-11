pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "PetControlRuntime.js" as PetControlRuntime

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
    property var petAssetSettings: null
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
    property string modelRoot: petAssetSettings ? petAssetSettings.modelRoot : ""
    property string modelJson: petAssetSettings ? petAssetSettings.modelJson : ""
    property string motionDirectory: petAssetSettings ? petAssetSettings.motionDirectory : ""
    property string expressionDirectory: petAssetSettings ? petAssetSettings.expressionDirectory : ""
    property string voiceDirectory: petAssetSettings ? petAssetSettings.voiceDirectory : ""

    readonly property color accentColor: "#74b2ba"
    readonly property color panelColor: "#fffafd"
    readonly property color borderColor: "#ead6e1"
    readonly property var availableModels: agentController ? agentController.availableModels : []
    readonly property string currentModel: agentController ? agentController.currentModel : ""
    readonly property bool apiProviderBusy: agentController ? agentController.apiProviderBusy : false
    readonly property string apiProviderStatus: agentController ? agentController.apiProviderStatus : ""
    readonly property bool modelRefreshBusy: agentController ? agentController.modelRefreshBusy : false
    readonly property var live2dActions: live2dActionAsset.actionItems

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
    signal live2DActionRequested(var action)
    signal live2DAutoRequested()

    Live2DAsset {
        id: live2dActionAsset
        modelRoot: root.modelRoot
        modelJson: root.modelJson
        motionDirectory: root.motionDirectory
        expressionDirectory: root.expressionDirectory
        voiceDirectory: root.voiceDirectory
        onAssetInputsChanged: live2dActionRefresh.restart()
    }

    Timer {
        id: live2dActionRefresh
        interval: 160
        repeat: false
        onTriggered: live2dActionAsset.validate()
    }

    Component.onCompleted: live2dActionRefresh.start()

    function openPanel() {
        show()
        raise()
        requestActivate()
        if (root.agentController) {
            root.agentController.refreshModelList()
        }
    }

    function displayStatus() {
        return PetControlRuntime.displayStatus(root.petController)
    }

    function modelProviderAvailable() {
        return PetControlRuntime.modelProviderAvailable(root.providerAvailable,
                                                        root.currentModel,
                                                        root.availableModels,
                                                        root.apiProviderStatus)
    }

    function cloudVisionRuntimeEnabled() {
        return PetControlRuntime.cloudVisionRuntimeEnabled(root.cloudVisionEnabled,
                                                          root.localOnlyMode,
                                                          root.modelProviderAvailable())
    }

    function cameraDiagnosticText() {
        return PetControlRuntime.cameraDiagnosticText(root.cameraEnabled,
                                                     root.cameraCaptureStatus)
    }

    function cloudVisionDiagnosticText() {
        return PetControlRuntime.cloudVisionDiagnosticText(root.localOnlyMode,
                                                          root.cloudVisionEnabled,
                                                          root.modelProviderAvailable())
    }

    function retentionDiagnosticText() {
        return PetControlRuntime.retentionDiagnosticText(root.debugRetentionEnabled)
    }

    function requestLocalOnlyMode(checked) {
        if (checked && root.cloudVisionEnabled) {
            root.cloudVisionToggled(false)
        }
        root.localOnlyModeToggled(checked)
    }

    function requestCloudVision(checked) {
        if (checked && (root.localOnlyMode || !root.modelProviderAvailable())) {
            root.cloudVisionToggled(false)
            return
        }
        root.cloudVisionToggled(checked)
    }

    function sendQuickText(text) {
        if (!root.petController || text.length === 0) {
            return
        }
        root.petController.sendText(text)
    }

    function requestLive2DAction(action) {
        if (!action) {
            return
        }
        root.live2DActionRequested(action)
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

            PetControlHeader {
                Layout.fillWidth: true
                statusText: root.displayStatus()
                hasError: root.petController && root.petController.error.length > 0
                borderColor: "#dcc8d4"
                onCloseRequested: root.hide()
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

                    PetControlLive2DActionPanel {
                        actionItems: live2dActionAsset.actionItems
                        statusText: live2dActionAsset.statusText
                        assetAvailable: root.petAssetSettings !== null
                        borderColor: root.borderColor
                        onActionRequested: function(action) { root.requestLive2DAction(action) }
                        onAutoRequested: root.live2DAutoRequested()
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
                        enabled: !root.localOnlyMode && root.modelProviderAvailable()
                        checked: root.cloudVisionRuntimeEnabled()
                        onToggled: function(checked) { root.requestCloudVision(checked) }
                    }

                    OptionSwitch {
                        Layout.fillWidth: true
                        text: "本地优先"
                        checked: root.localOnlyMode
                        onToggled: function(checked) { root.requestLocalOnlyMode(checked) }
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

                    PetVisionPrivacyCard {
                        cameraDiagnosticText: root.cameraDiagnosticText()
                        cloudVisionDiagnosticText: root.cloudVisionDiagnosticText()
                        retentionDiagnosticText: root.retentionDiagnosticText()
                        cloudVisionEnabled: root.cloudVisionRuntimeEnabled()
                        debugRetentionEnabled: root.debugRetentionEnabled
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
                            to: 3.2
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

                    PetControlApiProviderPanel {
                        availableModels: root.availableModels
                        currentModel: root.currentModel
                        apiProviderStatus: root.apiProviderStatus
                        apiProviderBusy: root.apiProviderBusy
                        modelRefreshBusy: root.modelRefreshBusy
                        agentAvailable: root.agentController !== null
                        accentColor: root.accentColor
                        borderColor: root.borderColor
                        onRegisterRequested: function(providerName, baseUrl, apiKey) {
                            if (root.agentController && !root.apiProviderBusy) {
                                root.agentController.registerApiProvider(providerName, baseUrl, apiKey)
                            }
                        }
                        onRefreshRequested: function() {
                            if (root.agentController) {
                                root.agentController.refreshModelList()
                            }
                        }
                        onModelSelected: function(modelType, modelName) {
                            if (root.agentController) {
                                root.agentController.switchModel(modelType, modelName)
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
        opacity: enabled ? 1.0 : 0.45

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
