pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property var participants: []
    property string selfName: ""
    property bool expanded: false

    signal participantEditRequested(var participant)

    Layout.fillWidth: true
    Layout.preferredHeight: expanded ? 78 : 0
    visible: expanded
    radius: 12
    color: Qt.rgba(1, 1, 1, 0.16)
    border.color: Qt.rgba(1, 1, 1, 0.36)
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
            readonly property bool editableAgent: (modelData.kind || "") === "agent"

            width: 148
            height: ListView.view.height
            radius: 8
            color: participantMouse.pressed && editableAgent
                   ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                   : participantMouse.containsMouse && editableAgent
                     ? Qt.rgba(1, 1, 1, 0.48)
                     : Qt.rgba(1, 1, 1, 0.34)
            border.color: participantMouse.containsMouse && editableAgent
                          ? Qt.rgba(0.35, 0.61, 0.90, 0.42)
                          : Qt.rgba(1, 1, 1, 0.34)

            MouseArea {
                id: participantMouse
                anchors.fill: parent
                hoverEnabled: participantDelegate.editableAgent
                enabled: participantDelegate.editableAgent
                cursorShape: participantDelegate.editableAgent ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: root.participantEditRequested(participantDelegate.modelData)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: (participantDelegate.modelData.kind || "") === "self" && root.selfName.length > 0
                          ? root.selfName
                          : (participantDelegate.modelData.display_name || participantDelegate.modelData.name || "成员")
                    color: "#263448"
                    font.pixelSize: 12
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: (participantDelegate.modelData.kind || "agent") === "self" ? "你" : ((participantDelegate.modelData.kind || "agent") === "agent" ? "AI" : "成员")
                    color: "#6a7b92"
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }
            }
        }
    }
}
