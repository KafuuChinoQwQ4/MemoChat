import QtQuick 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

Rectangle {
    id: root

    property string title: ""
    property string itemId: ""
    property string statusText: ""
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"

    width: ListView.view ? ListView.view.width : 360
    height: 72
    radius: 8
    color: officialSourceMouse.containsMouse ? Qt.rgba(0.94, 0.97, 1.0, 0.74) : "transparent"
    border.color: "transparent"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 18
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3

            Text {
                Layout.fillWidth: true
                text: root.title || root.itemId || "漫画源"
                color: root.textPrimaryColor
                font.pixelSize: 17
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.statusText
                color: root.textSecondaryColor
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }

    }

    MouseArea {
        id: officialSourceMouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
