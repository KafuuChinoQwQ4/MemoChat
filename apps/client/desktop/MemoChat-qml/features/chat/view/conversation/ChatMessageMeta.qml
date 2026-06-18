pragma ComponentBehavior: Bound

import QtQuick 2.15
import "."

Item {
    id: root

    property bool outgoing: false
    property bool showSenderName: false
    property string senderName: ""
    property string translationText: ""
    property string messageState: "sent"
    property real bubbleX: 0
    property real bubbleY: 0
    property real bubbleWidth: 0
    property real bubbleHeight: 0
    property real contentMaxWidth: 240
    property real translationPreferredWidth: 120

    readonly property int translationBlockHeight: root.translationText.length > 0 ? (translationBubble.implicitHeight + 6) : 0
    readonly property int stateBlockHeight: stateBadge.active ? 16 : 0

    Text {
        id: senderLabel
        visible: root.showSenderName
        x: root.outgoing ? root.bubbleX + root.bubbleWidth - width : root.bubbleX
        y: root.bubbleY - height - 2
        text: root.senderName
        color: "#4f6078"
        font.pixelSize: 11
    }

    Rectangle {
        id: translationBubble
        visible: root.translationText.length > 0
        x: root.outgoing ? root.bubbleX + root.bubbleWidth - width : root.bubbleX
        y: root.bubbleY + root.bubbleHeight + 6
        width: root.translationPreferredWidth
        height: implicitHeight
        implicitHeight: translationColumn.implicitHeight + 14
        radius: 9
        color: Qt.rgba(0.89, 0.94, 1.0, 0.54)
        border.color: Qt.rgba(0.42, 0.62, 0.86, 0.38)

        Column {
            id: translationColumn
            anchors.fill: parent
            anchors.margins: 7
            spacing: 3

            Text {
                text: "翻译"
                color: "#4f6788"
                font.pixelSize: 10
                font.bold: true
            }

            Text {
                text: root.translationText
                width: Math.min(root.contentMaxWidth, implicitWidth)
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                color: "#253247"
                font.pixelSize: 13
            }
        }
    }

    ChatMessageStatusBadge {
        id: stateBadge
        outgoing: root.outgoing
        messageState: root.messageState
        x: root.bubbleX + root.bubbleWidth - width
        y: root.translationText.length > 0
           ? translationBubble.y + translationBubble.height + 3
           : root.bubbleY + root.bubbleHeight + 3
    }
}
