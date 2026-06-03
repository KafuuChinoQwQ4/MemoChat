pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "PetControlRuntime.js" as PetControlRuntime

ColumnLayout {
    id: root

    property var actionItems: []
    property string statusText: ""
    property bool assetAvailable: false
    property color borderColor: "#ead6e1"

    signal actionRequested(var action)
    signal autoRequested()

    Layout.fillWidth: true
    spacing: 7

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: "动作"
            color: "#4b3042"
            font.pixelSize: 13
            font.bold: true
            elide: Text.ElideRight
        }

        Label {
            text: root.actionItems.length + " 个"
            color: "#6a7b92"
            font.pixelSize: 11
        }

        ActionButton {
            Layout.preferredWidth: 70
            text: "自动"
            onClicked: root.autoRequested()
        }
    }

    Label {
        Layout.fillWidth: true
        visible: root.actionItems.length === 0
        text: root.statusText.length > 0 ? root.statusText : "未发现可用动作"
        color: "#8f7c88"
        font.pixelSize: 11
        wrapMode: Text.Wrap
        maximumLineCount: 2
        elide: Text.ElideRight
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        rowSpacing: 6
        columnSpacing: 6

        Repeater {
            model: root.actionItems

            delegate: ActionButton {
                required property var modelData
                Layout.fillWidth: true
                text: (modelData.name || modelData.trigger || "")
                      + " · " + PetControlRuntime.actionKindLabel(modelData.kind || "")
                enabled: root.assetAvailable
                onClicked: root.actionRequested(modelData)
            }
        }
    }

    component ActionButton: Button {
        id: button
        font.pixelSize: 12
        padding: 8
        background: Rectangle {
            radius: 8
            antialiasing: true
            color: button.down ? "#d6f0ed" : button.hovered ? "#e8f6f4" : "#fffafd"
            border.color: button.enabled ? root.borderColor : "#eadfe6"
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
}
