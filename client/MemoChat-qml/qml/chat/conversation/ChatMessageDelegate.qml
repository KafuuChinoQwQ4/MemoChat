import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root
    width: ListView.view ? ListView.view.width : 300

    property bool outgoing: false
    property string msgType: "text"
    property string content: ""
    property string fileName: ""
    property bool showAvatar: true
    property string avatarSource: "qrc:/res/head_1.jpg"
    signal openUrlRequested(string url)

    property int avatarSize: 34
    property int avatarSlotWidth: 42
    property int topSpacing: showAvatar ? 8 : 2
    property int bottomSpacing: 2
    readonly property real bubbleMaxWidth: Math.max(120, width - avatarSlotWidth - 20)
    readonly property real messageHeight: Math.max(bubble.implicitHeight, showAvatar ? avatarSize : 0)

    height: topSpacing + messageHeight + bottomSpacing

    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: root.topSpacing
        height: root.messageHeight

        Item {
            id: leftSlot
            anchors.left: parent.left
            width: root.avatarSlotWidth
            height: parent.height

            Rectangle {
                width: root.avatarSize
                height: root.avatarSize
                radius: width / 2
                clip: true
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                visible: !root.outgoing && root.showAvatar
                color: Qt.rgba(0.73, 0.82, 0.92, 0.50)
                border.color: Qt.rgba(1, 1, 1, 0.56)

                Image {
                    anchors.fill: parent
                    source: root.avatarSource.length > 0 ? root.avatarSource : "qrc:/res/head_1.jpg"
                    fillMode: Image.PreserveAspectCrop
                }
            }
        }

        Item {
            id: rightSlot
            anchors.right: parent.right
            width: root.avatarSlotWidth
            height: parent.height

            Rectangle {
                width: root.avatarSize
                height: root.avatarSize
                radius: width / 2
                clip: true
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                visible: root.outgoing && root.showAvatar
                color: Qt.rgba(0.73, 0.82, 0.92, 0.50)
                border.color: Qt.rgba(1, 1, 1, 0.56)

                Image {
                    anchors.fill: parent
                    source: root.avatarSource.length > 0 ? root.avatarSource : "qrc:/res/head_1.jpg"
                    fillMode: Image.PreserveAspectCrop
                }
            }
        }
    }

    Rectangle {
        id: bubble
        width: Math.min(root.bubbleMaxWidth, contentItem.implicitWidth + 26)
        implicitHeight: contentItem.implicitHeight + 16
        radius: 10
        color: root.outgoing ? Qt.rgba(0.62, 0.80, 1.0, 0.52) : Qt.rgba(1, 1, 1, 0.50)
        border.color: root.outgoing ? Qt.rgba(0.44, 0.67, 0.95, 0.82) : Qt.rgba(1, 1, 1, 0.66)
        anchors.top: parent.top
        anchors.topMargin: root.topSpacing
        anchors.right: root.outgoing ? parent.right : undefined
        anchors.rightMargin: root.outgoing ? root.avatarSlotWidth : 0
        anchors.left: root.outgoing ? undefined : parent.left
        anchors.leftMargin: root.outgoing ? 0 : root.avatarSlotWidth

        Loader {
            id: contentItem
            anchors.fill: parent
            anchors.margins: 8
            sourceComponent: {
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
        }
    }

    Component {
        id: textComp
        Text {
            text: root.content
            wrapMode: Text.Wrap
            color: "#213045"
            font.pixelSize: 14
        }
    }

    Component {
        id: imageComp
        Item {
            implicitWidth: img.status === Image.Ready ? Math.min(220, img.implicitWidth) : 200
            implicitHeight: img.status === Image.Ready ? Math.min(160, img.implicitHeight) : 120

            Image {
                id: img
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                source: root.content
            }
        }
    }

    Component {
        id: fileComp
        Rectangle {
            color: "transparent"
            implicitWidth: Math.min(280, fileText.implicitWidth + 34)
            implicitHeight: fileText.implicitHeight + 8

            Text {
                id: fileText
                anchors.verticalCenter: parent.verticalCenter
                text: "[FILE] " + (root.fileName.length > 0 ? root.fileName : "文件")
                color: "#233247"
                font.pixelSize: 14
                wrapMode: Text.Wrap
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.openUrlRequested(root.content)
            }
        }
    }

    Component {
        id: callComp
        Rectangle {
            color: "transparent"
            implicitWidth: 220
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
