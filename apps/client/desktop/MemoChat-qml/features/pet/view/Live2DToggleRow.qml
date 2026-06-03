import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property alias checked: optionSwitch.checked
    property color textPrimaryColor: "#253247"
    property color textMutedColor: "#6e7f95"

    implicitHeight: Math.max(60, row.implicitHeight + 14)
    radius: 8
    color: Qt.rgba(1, 1, 1, 0.22)
    border.color: Qt.rgba(1, 1, 1, 0.34)

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.title
                color: root.textPrimaryColor
                font.pixelSize: 13
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: root.subtitle
                color: root.textMutedColor
                font.pixelSize: 11
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }

        Switch {
            id: optionSwitch
            Layout.preferredWidth: 56
        }
    }
}
