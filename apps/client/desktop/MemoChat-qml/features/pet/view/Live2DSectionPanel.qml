import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property color accentColor: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    property color textPrimaryColor: "#253247"
    property color textMutedColor: "#6e7f95"
    default property alias content: body.data

    Layout.fillWidth: true
    implicitHeight: panelLayout.implicitHeight + 24
    radius: 8
    color: Qt.rgba(1, 1, 1, 0.24)
    border.color: Qt.rgba(1, 1, 1, 0.42)

    ColumnLayout {
        id: panelLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 4
                Layout.preferredHeight: 28
                radius: 2
                color: root.accentColor
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: root.title
                    color: root.textPrimaryColor
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: root.subtitle
                    visible: text.length > 0
                    color: root.textMutedColor
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }
            }
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: 10
        }
    }
}
