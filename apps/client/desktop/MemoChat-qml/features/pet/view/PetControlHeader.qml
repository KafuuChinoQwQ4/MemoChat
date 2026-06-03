import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: root

    property string statusText: "桌宠"
    property bool hasError: false
    property color borderColor: "#ead6e1"
    signal closeRequested()

    spacing: 8

    Rectangle {
        Layout.preferredWidth: 9
        Layout.preferredHeight: 9
        radius: 5
        color: root.hasError ? "#e35b5b" : "#74b2ba"
    }

    Label {
        Layout.fillWidth: true
        text: root.statusText
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
        onClicked: root.closeRequested()
        background: Rectangle {
            radius: 14
            antialiasing: true
            color: panelCloseButton.down ? "#f1d7e3"
                                         : panelCloseButton.hovered ? "#f8e6ee" : "#fffafd"
            border.color: root.borderColor
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
