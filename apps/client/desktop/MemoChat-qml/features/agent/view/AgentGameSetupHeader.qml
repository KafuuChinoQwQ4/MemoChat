import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

RowLayout {
    id: root
    spacing: 10

    property string title: ""
    property string hint: ""
    property bool error: false

    signal closeRequested()

    ColumnLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: 3

        Label {
            Layout.fillWidth: true
            text: root.title
            color: "#243145"
            font.pixelSize: 20
            font.bold: true
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: root.hint
            color: root.error ? "#b64a4a" : "#65758b"
            font.pixelSize: 12
            elide: Text.ElideRight
        }
    }

    GlassButton {
        Layout.preferredWidth: 72
        Layout.preferredHeight: 32
        text: "关闭"
        textPixelSize: 12
        cornerRadius: 8
        normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.20)
        hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.30)
        onClicked: root.closeRequested()
    }
}
