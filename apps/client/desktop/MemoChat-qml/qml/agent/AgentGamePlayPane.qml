pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

ColumnLayout {
    id: root

    property var agentController: null
    property var participants: []
    property var events: []
    property var availableActions: []
    property bool gameBusy: false
    property bool roomEnded: false
    property string roomId: ""
    property string roomLabel: "未选择房间"

    signal refreshRequested()
    signal startRequested()
    signal restartRequested()
    signal tickRequested()
    signal autoTickRequested()
    signal submitActionRequested(string actorId, string actionType, string targetId, string content)

    function participantName(participantId) {
        for (var i = 0; i < root.participants.length; ++i) {
            var p = root.participants[i]
            if ((p.participant_id || p.id || "") === participantId) {
                return p.display_name || p.name || participantId
            }
        }
        return participantId || "系统"
    }

    function eventTitle(event) {
        var actor = participantName(event.actor_participant_id || "")
        var type = event.event_type || event.type || "event"
        return actor + " · " + type
    }

    function actorIdAt(index) {
        if (index < 0 || index >= root.participants.length) {
            return ""
        }
        return root.participants[index].participant_id || root.participants[index].id || ""
    }

    function actionTypeAt(index) {
        if (index < 0 || index >= root.availableActions.length) {
            return "say"
        }
        var action = root.availableActions[index]
        if (typeof action === "string") {
            return action
        }
        return action.action_type || action.type || "say"
    }

    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumWidth: 0
    spacing: 10

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        GlassButton {
            Layout.preferredWidth: 70
            Layout.preferredHeight: 34
            text: "刷新"
            textPixelSize: 12
            cornerRadius: 8
            normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.20)
            hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.30)
            onClicked: root.refreshRequested()
        }

        GlassButton {
            Layout.preferredWidth: 82
            Layout.preferredHeight: 34
            text: root.roomEnded ? "重新开始" : "开始"
            textPixelSize: 12
            cornerRadius: 8
            enabled: !root.gameBusy && root.roomId.length > 0
            normalColor: Qt.rgba(0.30, 0.58, 0.36, 0.24)
            hoverColor: Qt.rgba(0.30, 0.58, 0.36, 0.34)
            onClicked: root.roomEnded ? root.restartRequested() : root.startRequested()
        }

        GlassButton {
            Layout.preferredWidth: 70
            Layout.preferredHeight: 34
            text: "Tick"
            textPixelSize: 12
            cornerRadius: 8
            enabled: !root.gameBusy && root.roomId.length > 0
            normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.22)
            hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.32)
            onClicked: root.tickRequested()
        }

        GlassButton {
            Layout.preferredWidth: 70
            Layout.preferredHeight: 34
            text: "Auto"
            textPixelSize: 12
            cornerRadius: 8
            enabled: !root.gameBusy && root.roomId.length > 0
            normalColor: Qt.rgba(0.48, 0.42, 0.72, 0.22)
            hoverColor: Qt.rgba(0.48, 0.42, 0.72, 0.32)
            onClicked: root.autoTickRequested()
        }

        Item { Layout.fillWidth: true }

        Label {
            Layout.maximumWidth: 220
            text: root.roomLabel
            color: "#65758b"
            font.pixelSize: 11
            elide: Text.ElideMiddle
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 104
        radius: 10
        color: Qt.rgba(1, 1, 1, 0.32)
        border.color: Qt.rgba(1, 1, 1, 0.40)
        clip: true

        ListView {
            anchors.fill: parent
            anchors.margins: 10
            orientation: ListView.Horizontal
            spacing: 8
            model: root.participants
            clip: true

            delegate: Rectangle {
                id: participantDelegate
                required property var modelData

                width: 154
                height: ListView.view.height
                radius: 8
                color: Qt.rgba(1, 1, 1, 0.38)
                border.color: Qt.rgba(1, 1, 1, 0.36)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 9
                    spacing: 3

                    Label { Layout.fillWidth: true; text: participantDelegate.modelData.display_name || participantDelegate.modelData.name || "玩家"; color: "#243145"; font.pixelSize: 13; font.bold: true; elide: Text.ElideRight }
                    Label { Layout.fillWidth: true; text: (participantDelegate.modelData.kind || participantDelegate.modelData.type || "participant") + " · " + (participantDelegate.modelData.status || "ready"); color: "#65758b"; font.pixelSize: 11; elide: Text.ElideRight }
                    Label { Layout.fillWidth: true; text: participantDelegate.modelData.role_key || participantDelegate.modelData.role || "role"; color: "#4d6d88"; font.pixelSize: 11; elide: Text.ElideRight }
                }
            }

            Label {
                anchors.centerIn: parent
                text: "创建或选择房间"
                color: "#65758b"
                font.pixelSize: 12
                visible: root.participants.length === 0
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        radius: 10
        color: Qt.rgba(1, 1, 1, 0.30)
        border.color: Qt.rgba(1, 1, 1, 0.40)
        clip: true

        ListView {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8
            model: root.events
            clip: true

            delegate: Item {
                id: eventDelegate
                required property var modelData

                width: ListView.view.width
                height: Math.max(58, bubbleColumn.implicitHeight + 16)

                Rectangle {
                    width: Math.min(parent.width - 20, Math.max(260, bubbleColumn.implicitWidth + 28))
                    height: bubbleColumn.implicitHeight + 16
                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.42)
                    border.color: Qt.rgba(1, 1, 1, 0.34)

                    ColumnLayout {
                        id: bubbleColumn
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 3

                        Label {
                            Layout.fillWidth: true
                            text: root.eventTitle(eventDelegate.modelData)
                            color: "#243145"
                            font.pixelSize: 12
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: eventDelegate.modelData.content || eventDelegate.modelData.message || JSON.stringify(eventDelegate.modelData)
                            color: "#53637a"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                            maximumLineCount: 8
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 28
                text: "时间线会显示在这里。"
                color: "#65758b"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: root.events.length === 0
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 108
        radius: 10
        color: Qt.rgba(1, 1, 1, 0.34)
        border.color: Qt.rgba(1, 1, 1, 0.40)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 7

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ComboBox {
                    id: actorCombo
                    Layout.preferredWidth: 164
                    Layout.preferredHeight: 32
                    textRole: "label"
                    valueRole: "value"
                    model: {
                        var rows = []
                        for (var i = 0; i < root.participants.length; ++i) {
                            var p = root.participants[i]
                            rows.push({ "label": p.display_name || p.name || p.participant_id || "玩家", "value": p.participant_id || p.id || "" })
                        }
                        if (rows.length === 0) rows.push({ "label": "选择行动者", "value": "" })
                        return rows
                    }
                }

                ComboBox {
                    id: actionCombo
                    Layout.preferredWidth: 128
                    Layout.preferredHeight: 32
                    textRole: "label"
                    valueRole: "value"
                    model: {
                        var rows = []
                        for (var i = 0; i < root.availableActions.length; ++i) {
                            var action = root.availableActions[i]
                            if (typeof action === "string") rows.push({ "label": action, "value": action })
                            else rows.push({ "label": action.label || action.name || action.action_type || "行动", "value": action.action_type || action.type || "say" })
                        }
                        if (rows.length === 0) rows.push({ "label": "发言", "value": "say" })
                        return rows
                    }
                }

                TextField {
                    id: targetField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    placeholderText: "目标 ID"
                    selectByMouse: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    id: actionTextField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    placeholderText: "真人玩家发言 / 行动"
                    selectByMouse: true
                    onAccepted: submitActionButton.clicked()
                }

                GlassButton {
                    id: submitActionButton
                    Layout.preferredWidth: 82
                    Layout.preferredHeight: 34
                    text: "提交"
                    textPixelSize: 12
                    cornerRadius: 8
                    enabled: !root.gameBusy && root.roomId.length > 0
                    normalColor: Qt.rgba(0.33, 0.56, 0.84, 0.22)
                    hoverColor: Qt.rgba(0.33, 0.56, 0.84, 0.32)
                    onClicked: {
                        root.submitActionRequested(root.actorIdAt(actorCombo.currentIndex),
                                                   root.actionTypeAt(actionCombo.currentIndex),
                                                   targetField.text,
                                                   actionTextField.text)
                        actionTextField.text = ""
                    }
                }
            }
        }
    }
}
