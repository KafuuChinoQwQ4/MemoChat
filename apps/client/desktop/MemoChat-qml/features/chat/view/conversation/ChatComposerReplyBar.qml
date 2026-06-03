import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

GlassSurface {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: root.hasReplyContext ? 44 : 0
    visible: root.hasReplyContext
    cornerRadius: 9
    blurRadius: 14
    fillColor: Qt.rgba(0.30, 0.39, 0.52, 0.18)
    strokeColor: Qt.rgba(0.61, 0.74, 0.92, 0.36)

    property bool hasReplyContext: false
    property string replyTargetName: ""
    property string replyPreviewText: ""

    signal clearRequested()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 8
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 3
            Layout.fillHeight: true
            radius: 2
            color: Qt.rgba(0.40, 0.66, 0.96, 0.74)
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1
            Label {
                text: root.replyTargetName.length > 0 ? ("回复 " + root.replyTargetName) : "回复"
                color: "#38547b"
                font.pixelSize: 12
                font.bold: true
            }
            Label {
                text: root.replyPreviewText
                color: "#556d89"
                font.pixelSize: 12
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        ToolButton {
            id: clearReplyButton
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            hoverEnabled: true
            scale: down ? 0.96 : (hovered ? 1.02 : 1.0)
            onClicked: root.clearRequested()

            Behavior on scale {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }

            HoverHandler {
                cursorShape: clearReplyButton.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
            contentItem: Label {
                text: "x"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#4d5f76"
                font.pixelSize: 13
            }
            background: Rectangle {
                radius: 7
                color: clearReplyButton.down ? Qt.rgba(1, 1, 1, 0.34)
                                             : clearReplyButton.hovered ? Qt.rgba(1, 1, 1, 0.24)
                                                                       : Qt.rgba(1, 1, 1, 0.16)
                border.color: Qt.rgba(1, 1, 1, 0.30)

                Behavior on color {
                    ColorAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }
            }
        }
    }
}
