import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root

    property string title: ""
    property alias text: area.text
    property string placeholderText: ""
    property int preferredHeight: 80
    property color textPrimaryColor: "#253247"
    property color textSecondaryColor: "#4e5d74"
    property color textMutedColor: "#6e7f95"

    Layout.fillWidth: true
    spacing: 5

    Label {
        Layout.fillWidth: true
        text: root.title
        color: root.textSecondaryColor
        font.pixelSize: 12
        font.bold: true
        elide: Text.ElideRight
    }

    TextArea {
        id: area
        Layout.fillWidth: true
        Layout.preferredHeight: root.preferredHeight
        placeholderText: root.placeholderText
        placeholderTextColor: root.textMutedColor
        color: root.textPrimaryColor
        font.pixelSize: 13
        wrapMode: TextArea.Wrap
        selectByMouse: true
        background: Rectangle {
            radius: 8
            color: Qt.rgba(1, 1, 1, 0.34)
            border.width: 1
            border.color: area.activeFocus ? Qt.rgba(0.35, 0.61, 0.90, 0.46)
                                           : Qt.rgba(1, 1, 1, 0.42)
        }
    }
}
