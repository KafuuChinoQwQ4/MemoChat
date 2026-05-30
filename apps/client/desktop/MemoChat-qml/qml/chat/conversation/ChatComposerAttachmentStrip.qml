pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    Layout.fillWidth: true
    Layout.preferredHeight: root.hasPendingAttachmentItems ? 76 : 0
    visible: root.hasPendingAttachmentItems
    radius: 10
    color: Qt.rgba(0.88, 0.93, 0.98, 0.42)
    border.color: Qt.rgba(0.49, 0.67, 0.89, 0.30)

    property var pendingAttachments: []
    property bool mediaUploadInProgress: false
    readonly property bool hasPendingAttachmentItems: pendingAttachments && pendingAttachments.length > 0

    signal removeRequested(string attachmentId)

    Flickable {
        anchors.fill: parent
        anchors.margins: 8
        clip: true
        contentWidth: attachmentRow.implicitWidth
        contentHeight: attachmentRow.height

        Row {
            id: attachmentRow
            spacing: 8

            Repeater {
                model: root.pendingAttachments

                delegate: Rectangle {
                    width: modelData.type === "image" ? 96 : 164
                    height: 52
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.50)
                    border.color: Qt.rgba(0.44, 0.61, 0.82, 0.28)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 4
                        spacing: 6

                        Rectangle {
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                            radius: 6
                            color: Qt.rgba(0.80, 0.88, 0.96, 0.65)
                            border.color: Qt.rgba(1, 1, 1, 0.44)
                            clip: true

                            Image {
                                anchors.fill: parent
                                anchors.margins: modelData.type === "image" ? 0 : 10
                                source: modelData.type === "image" ? modelData.previewUrl : "qrc:/icons/file.png"
                                fillMode: modelData.type === "image" ? Image.PreserveAspectCrop : Image.PreserveAspectFit
                                sourceSize.width: 80
                                sourceSize.height: 80
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Label {
                                Layout.fillWidth: true
                                text: modelData.fileName
                                color: "#2a3950"
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }

                            Label {
                                Layout.fillWidth: true
                                text: modelData.type === "image" ? "图片待发送" : "文件待发送"
                                color: "#667b95"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        ToolButton {
                            id: removeButton
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            enabled: !root.mediaUploadInProgress
                            onClicked: root.removeRequested(modelData.attachmentId)
                            contentItem: Label {
                                text: "x"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: "#516378"
                                font.pixelSize: 12
                            }
                            background: Rectangle {
                                radius: 6
                                color: removeButton.down ? Qt.rgba(0.35, 0.61, 0.90, 0.18)
                                                         : removeButton.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.12)
                                                                                : "transparent"
                            }
                        }
                    }
                }
            }
        }
    }
}
