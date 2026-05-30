import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../components"

Rectangle {
    id: root

    property int sourceIndex: -1
    property string title: ""
    property string itemId: ""
    property string statusText: ""
    property color textPrimaryColor: "#263241"
    property color textSecondaryColor: "#4e6072"
    property color importButtonColor: "#0c4f92"
    property color importButtonHoverColor: "#0f61b0"
    property color importButtonPressedColor: "#093d72"

    signal importRequested(int sourceIndex)

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

        GlassButton {
            Layout.preferredWidth: 96
            Layout.preferredHeight: 40
            text: "添加"
            textPixelSize: 15
            textColor: "#ffffff"
            cornerRadius: 20
            normalColor: root.importButtonColor
            hoverColor: root.importButtonHoverColor
            pressedColor: root.importButtonPressedColor
            onClicked: root.importRequested(root.sourceIndex)
        }
    }

    MouseArea {
        id: officialSourceMouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
