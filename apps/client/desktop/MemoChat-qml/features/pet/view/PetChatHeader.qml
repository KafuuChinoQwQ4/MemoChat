import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property string displayName: "Live2D"
    property string chatStatusText: "准备聊天"
    property string avatarSource: "qrc:/icons/modelive2d.png"
    property bool alwaysOnTop: false
    property bool clickThrough: false
    property bool voiceCallActive: false
    property bool videoCallActive: false
    property color accentColor: "#74b2ba"

    signal alwaysOnTopToggled()
    signal clickThroughToggled()
    signal voiceCallToggled()
    signal videoCallToggled()
    signal closeRequested()

    Layout.fillWidth: true
    Layout.preferredHeight: 44
    radius: 12
    antialiasing: true
    color: Qt.rgba(1, 1, 1, 0.30)
    border.color: Qt.rgba(1, 1, 1, 0.50)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 8
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 26
            Layout.preferredHeight: 26
            radius: 13
            antialiasing: true
            clip: true
            color: Qt.rgba(0.74, 0.86, 0.95, 0.72)
            border.color: Qt.rgba(1, 1, 1, 0.66)

            Image {
                anchors.fill: parent
                anchors.margins: 2
                source: root.avatarSource
                fillMode: Image.PreserveAspectCrop
                mipmap: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 0

            Label {
                Layout.fillWidth: true
                text: root.displayName
                color: "#26364d"
                font.pixelSize: 14
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: root.chatStatusText
                color: "#6a7b92"
                font.pixelSize: 10
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.preferredWidth: 8
            Layout.preferredHeight: 8
            Layout.alignment: Qt.AlignVCenter
            radius: 4
            antialiasing: true
            color: root.voiceCallActive || root.videoCallActive ? root.accentColor : "#8fc7d1"
        }

        ToolButton {
            id: closeButton
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            hoverEnabled: true
            padding: 0
            onClicked: root.closeRequested()

            contentItem: Text {
                text: "x"
                color: "#536174"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: 8
                antialiasing: true
                color: closeButton.down ? Qt.rgba(0.36, 0.52, 0.72, 0.22)
                                        : closeButton.hovered ? Qt.rgba(0.36, 0.52, 0.72, 0.14)
                                                              : "transparent"
            }
        }
    }
}
