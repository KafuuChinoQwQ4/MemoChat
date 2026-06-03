pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    radius: 18
    color: "#ffffff"
    border.color: "#dce4ef"
    border.width: 1

    property var attachmentsModel: null
    property int selectedMediaIndex: 0

    signal selectedMediaIndexChangedRequested(int index)
    signal removeAttachmentRequested(int index)
    signal pickMediaRequested()

    Flickable {
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        contentWidth: thumbRow.width
        contentHeight: height
        boundsBehavior: Flickable.StopAtBounds

        Row {
            id: thumbRow
            spacing: 8
            height: parent.height

            Repeater {
                model: root.attachmentsModel

                delegate: Rectangle {
                    id: attachmentThumb

                    required property int index
                    required property string type
                    required property string previewUrl
                    required property string fileUrl

                    width: 62
                    height: 62
                    radius: 14
                    color: index === root.selectedMediaIndex ? "#d8e6ff" : "#edf2f8"
                    border.width: 1
                    border.color: index === root.selectedMediaIndex ? "#4e87f6" : "#cfdae8"
                    clip: true

                    Image {
                        anchors.fill: parent
                        visible: attachmentThumb.type === "image"
                        source: visible ? (attachmentThumb.previewUrl || attachmentThumb.fileUrl || "") : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }

                    Rectangle {
                        anchors.fill: parent
                        visible: attachmentThumb.type === "video"
                        color: "#243245"

                        Label {
                            anchors.centerIn: parent
                            text: "▶"
                            color: "#ffffff"
                            font.pixelSize: 16
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.selectedMediaIndexChangedRequested(attachmentThumb.index)
                    }

                    Rectangle {
                        width: 18
                        height: 18
                        radius: 9
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 4
                        color: Qt.rgba(0.09, 0.12, 0.18, 0.78)

                        Label {
                            anchors.centerIn: parent
                            text: "×"
                            color: "#ffffff"
                            font.pixelSize: 12
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.removeAttachmentRequested(attachmentThumb.index)
                        }
                    }
                }
            }

            Rectangle {
                width: 62
                height: 62
                radius: 14
                color: "#ffffff"
                border.width: 1
                border.color: "#c7d3e3"

                Label {
                    anchors.centerIn: parent
                    text: "+"
                    font.pixelSize: 28
                    color: "#2b6ff2"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.pickMediaRequested()
                }
            }
        }
    }
}
