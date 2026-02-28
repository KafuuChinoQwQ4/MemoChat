import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    width: ListView.view ? ListView.view.width : 300

    property bool outgoing: false
    property string msgType: "text"
    property string content: ""
    property string fileName: ""
    signal openUrlRequested(string url)

    height: bubble.implicitHeight + 6

    Rectangle {
        id: bubble
        width: Math.min(parent.width * 0.68, contentItem.implicitWidth + 26)
        implicitHeight: contentItem.implicitHeight + 16
        radius: 8
        color: root.outgoing ? "#9fe0a8" : "#ffffff"
        border.color: root.outgoing ? "#8ad594" : "#d9e0ea"
        anchors.right: root.outgoing ? parent.right : undefined
        anchors.left: root.outgoing ? undefined : parent.left

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
            color: "#243142"
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
                color: "#2f3a4a"
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
                    color: "#2f3a4a"
                    font.pixelSize: 14
                    font.bold: true
                }

                Button {
                    text: "加入通话"
                    enabled: root.content.length > 0
                    onClicked: root.openUrlRequested(root.content)
                }
            }
        }
    }
}
