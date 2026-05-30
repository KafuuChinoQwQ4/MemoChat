import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root

    property bool active: false
    property string title: ""
    property int maxPanelWidth: 620
    property int maxPanelHeight: 640
    property Component contentComponent: null

    signal closeRequested()

    visible: active
    enabled: active

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(20, 28, 40, 0.22)

        MouseArea {
            anchors.fill: parent
            onClicked: root.closeRequested()
        }

        Rectangle {
            anchors.centerIn: parent
            width: Math.min(root.maxPanelWidth, Math.max(0, root.width - 32))
            height: Math.min(root.maxPanelHeight, Math.max(0, root.height - 32))
            radius: 14
            clip: true
            color: Qt.rgba(0.96, 0.98, 1.0, 0.84)
            border.color: Qt.rgba(1, 1, 1, 0.48)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: root.title
                        font.pixelSize: 16
                        font.bold: true
                        color: "#2a3649"
                        elide: Text.ElideRight
                    }

                    GlassButton {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 30
                        text: "关闭"
                        textPixelSize: 12
                        cornerRadius: 8
                        normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.24)
                        onClicked: root.closeRequested()
                    }
                }

                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sourceComponent: root.contentComponent
                }
            }
        }
    }
}
