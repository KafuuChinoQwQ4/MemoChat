import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

ColumnLayout {
    id: root

    property Item backdrop: null
    property string title: ""
    property alias text: input.text
    property string placeholderText: ""
    property color textSecondaryColor: "#4e5d74"

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

    GlassTextField {
        id: input
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        backdrop: root.backdrop
        textHorizontalAlignment: TextInput.AlignLeft
        textPixelSize: 13
        leftInset: 10
        rightInset: 10
        cornerRadius: 8
        placeholderText: root.placeholderText
        fillColor: Qt.rgba(1, 1, 1, 0.28)
        strokeColor: Qt.rgba(1, 1, 1, 0.46)
    }
}
