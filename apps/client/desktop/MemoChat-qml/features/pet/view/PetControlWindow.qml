pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import MemoChat 1.0
import "qrc:/qml/components"
import "../runtime/PetControlRuntime.js" as PetControlRuntime

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
    readonly property var apiProviderCandidates: agentController ? agentController.apiProviderCandidates : []
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

    function requestLive2DAction(action) {
        if (!action) {
            return
        }
        root.live2DActionRequested(action)
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 8
        anchors.topMargin: 12
        radius: 16
        antialiasing: true
        color: Qt.rgba(0.26, 0.16, 0.23, 0.16)
        z: -1
    }

    GlassSurface {
        anchors.fill: parent
        anchors.margins: 8
        cornerRadius: 16
        fillColor: Qt.rgba(0.995, 0.972, 0.997, 0.95)
        strokeColor: Qt.rgba(0.84, 0.74, 0.82, 0.55)
        strokeWidth: 1
        glowTopColor: Qt.rgba(1, 1, 1, 0.28)
        glowBottomColor: Qt.rgba(0.95, 0.90, 0.94, 0.07)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            Item {
                id: panelHeader
                Layout.fillWidth: true
                Layout.preferredHeight: 32

                DragHandler {
                    target: null
                    acceptedButtons: Qt.LeftButton
                    onActiveChanged: {
                        if (active) root.startSystemMove()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 8

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        antialiasing: true
                        color: (root.petController && root.petController.error.length > 0)
                               ? "#e35b5b" : root.accentColor
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.displayStatus()
                        color: "#4b3042"
                        font.pixelSize: 13
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    GlassButton {
                        width: 26
                        height: 26
                        text: "×"
                        textPixelSize: 17
                        textColor: "#6c4a5e"
                        cornerRadius: 13
                        normalColor: Qt.rgba(0, 0, 0, 0.05)
                        hoverColor: Qt.rgba(0.85, 0.55, 0.65, 0.16)
                        pressedColor: Qt.rgba(0.80, 0.45, 0.58, 0.28)
                        enableScaleFeedback: false
                        onClicked: root.hide()
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

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        text: "聊天"
                        textPixelSize: 12
                        textColor: enabled ? "#4b3042" : "#a997a3"
                        cornerRadius: 8
                        normalColor: Qt.rgba(0.91, 0.95, 0.95, 0.50)
                        hoverColor: Qt.rgba(0.83, 0.93, 0.94, 0.72)
                        pressedColor: Qt.rgba(0.72, 0.87, 0.89, 0.88)
                        disabledColor: Qt.rgba(0, 0, 0, 0.04)
                        enableScaleFeedback: false
                        enabled: root.petController !== null
                        onClicked: root.chatRequested()
                    }

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        text: "打断"
                        textPixelSize: 12
                        textColor: enabled ? "#4b3042" : "#a997a3"
                        cornerRadius: 8
                        normalColor: Qt.rgba(0.91, 0.95, 0.95, 0.50)
                        hoverColor: Qt.rgba(0.83, 0.93, 0.94, 0.72)
                        pressedColor: Qt.rgba(0.72, 0.87, 0.89, 0.88)
                        disabledColor: Qt.rgba(0, 0, 0, 0.04)
                        enableScaleFeedback: false
                        enabled: root.petController !== null && root.petController.sessionId.length > 0
                        onClicked: root.petController.interrupt()
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
                        apiProviderCandidates: root.apiProviderCandidates
                        modelRefreshBusy: root.modelRefreshBusy
                        agentAvailable: root.agentController !== null
                        accentColor: root.accentColor
                        borderColor: root.borderColor
                        onDiscoverRequested: function(providerName, baseUrl, apiKey) {
                            if (root.agentController && !root.apiProviderBusy) {
                                root.agentController.discoverApiProvider(providerName, baseUrl, apiKey)
                            }
                        }
                        onRegisterRequested: function(modelName) {
                            if (root.agentController && !root.apiProviderBusy && modelName.length > 0) {
                                root.agentController.registerDiscoveredApiModel(modelName)
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

                        GlassButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            text: "复位"
                            textPixelSize: 12
                            textColor: "#4b3042"
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.91, 0.95, 0.95, 0.50)
                            hoverColor: Qt.rgba(0.83, 0.93, 0.94, 0.72)
                            pressedColor: Qt.rgba(0.72, 0.87, 0.89, 0.88)
                            enableScaleFeedback: false
                            onClicked: root.resetPositionRequested()
                        }

                        GlassButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            text: "关闭桌宠"
                            textPixelSize: 12
                            textColor: "#4b3042"
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.91, 0.95, 0.95, 0.50)
                            hoverColor: Qt.rgba(0.83, 0.93, 0.94, 0.72)
                            pressedColor: Qt.rgba(0.72, 0.87, 0.89, 0.88)
                            enableScaleFeedback: false
                            onClicked: root.closePetRequested()
                        }
                    }
                }
            }
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
