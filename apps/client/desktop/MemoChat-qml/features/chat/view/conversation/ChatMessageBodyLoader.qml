pragma ComponentBehavior: Bound

import QtQuick 2.15
import "qrc:/qml/components"
import "."

Loader {
    id: root

    property string msgType: "text"
    property string content: ""
    property string fileName: ""
    property real textMaxWidth: 240
    property real fileMaxWidth: 240
    property real imageMaxWidth: 180
    property real imageMaxHeight: 240
    property real callMaxWidth: 220

    signal openUrlRequested(string url)

    asynchronous: false
    sourceComponent: bodyComponent()

    function bodyComponent() {
        if (root.msgType === "image") {
            return imageComp
        }
        if (root.msgType === "file") {
            return fileComp
        }
        if (root.msgType === "call") {
            return callComp
        }
        return textComp
    }

    Component {
        id: textComp
        ChatMessageTextBody {
            messageText: root.content
            maxWidth: root.textMaxWidth
        }
    }

    Component {
        id: imageComp
        ChatMessageImageBody {
            imageSource: root.content
            maxWidth: root.imageMaxWidth
            maxHeight: root.imageMaxHeight
        }
    }

    Component {
        id: fileComp
        ChatMessageFileBody {
            fileName: root.fileName
            maxWidth: root.fileMaxWidth
            width: implicitWidth
            height: implicitHeight
            onOpenRequested: root.openUrlRequested(root.content)
        }
    }

    Component {
        id: callComp
        Rectangle {
            color: "transparent"
            width: implicitWidth
            height: implicitHeight
            implicitWidth: Math.min(root.callMaxWidth, 220)
            implicitHeight: 62

            Column {
                anchors.fill: parent
                spacing: 6

                Text {
                    text: root.fileName.length > 0 ? root.fileName : "通话邀请"
                    color: "#233247"
                    font.pixelSize: 14
                    font.bold: true
                }

                GlassButton {
                    text: "加入通话"
                    implicitWidth: 92
                    implicitHeight: 30
                    cornerRadius: 8
                    normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                    hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                    pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                    enabled: root.content.length > 0
                    onClicked: root.openUrlRequested(root.content)
                }
            }
        }
    }
}
