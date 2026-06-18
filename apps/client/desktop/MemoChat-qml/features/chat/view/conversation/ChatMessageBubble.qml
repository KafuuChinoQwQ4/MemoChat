pragma ComponentBehavior: Bound

import QtQuick 2.15
import "."

Rectangle {
    id: root

    property bool outgoing: false
    property bool isReply: false
    property bool enableContextMenu: false
    property string msgType: "text"
    property string content: ""
    property string fileName: ""
    property string replySender: ""
    property string replyPreview: ""
    property real bubbleWidth: 120
    property real contentMaxWidth: 240
    property real imageMaxWidth: 180
    property real imageMaxHeight: 240
    property real horizontalPadding: 8
    property real verticalPadding: 8

    signal contextMenuRequested(real pointX, real pointY)
    signal openUrlRequested(string url)

    width: root.bubbleWidth
    height: implicitHeight
    implicitHeight: bubbleColumn.implicitHeight + root.verticalPadding * 2
    radius: 10
    color: root.outgoing ? Qt.rgba(0.62, 0.80, 1.0, 0.52) : Qt.rgba(1, 1, 1, 0.50)
    border.color: root.outgoing ? Qt.rgba(0.44, 0.67, 0.95, 0.82) : Qt.rgba(1, 1, 1, 0.66)

    Column {
        id: bubbleColumn
        anchors.fill: parent
        anchors.leftMargin: root.horizontalPadding
        anchors.rightMargin: root.horizontalPadding
        anchors.topMargin: root.verticalPadding
        anchors.bottomMargin: root.verticalPadding
        spacing: root.isReply ? 6 : 0

        Rectangle {
            visible: root.isReply
            width: Math.min(root.contentMaxWidth, replyColumn.implicitWidth + 10)
            height: visible ? replyColumn.implicitHeight + 8 : 0
            radius: 6
            color: Qt.rgba(0.25, 0.33, 0.46, 0.14)
            border.color: Qt.rgba(0.44, 0.60, 0.82, 0.40)

            Column {
                id: replyColumn
                anchors.fill: parent
                anchors.margins: 4
                spacing: 1

                Text {
                    text: root.replySender.length > 0 ? ("回复 " + root.replySender) : "回复"
                    color: "#4f6788"
                    font.pixelSize: 11
                    font.bold: true
                }

                Text {
                    text: root.replyPreview
                    color: "#60718a"
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                }
            }
        }

        ChatMessageBodyLoader {
            id: bodyLoader
            msgType: root.msgType
            content: root.content
            fileName: root.fileName
            textMaxWidth: root.contentMaxWidth
            fileMaxWidth: root.contentMaxWidth
            imageMaxWidth: root.imageMaxWidth
            imageMaxHeight: root.imageMaxHeight
            callMaxWidth: root.contentMaxWidth
            onOpenUrlRequested: function(url) { root.openUrlRequested(url) }
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        enabled: root.enableContextMenu
        onTapped: function(eventPoint, button) {
            if (button !== Qt.RightButton || !root.enableContextMenu) {
                return
            }
            root.contextMenuRequested(eventPoint.position.x, eventPoint.position.y)
        }
    }
}
