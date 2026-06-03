pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

GridLayout {
    id: root

    property var mediaTiles: []
    property var mediaUrlResolver: null
    property var mediaHeightResolver: null
    property var videoDurationFormatter: null

    signal imageRequested(string key)

    visible: mediaTiles && mediaTiles.length > 0
    columns: mediaTiles.length === 1 ? 1 : 3
    rowSpacing: 6
    columnSpacing: 6

    function mediaUrl(key) {
        return mediaUrlResolver ? mediaUrlResolver(key) : ""
    }

    function tileHeight(item) {
        return mediaHeightResolver ? mediaHeightResolver(item) : 120
    }

    function durationText(duration) {
        return videoDurationFormatter ? videoDurationFormatter(duration) : "点击打开视频"
    }

    Repeater {
        model: root.mediaTiles

        delegate: Rectangle {
            required property var modelData

            Layout.fillWidth: true
            Layout.preferredHeight: root.tileHeight(modelData)
            radius: 8
            color: Qt.rgba(0.93, 0.95, 0.98, 1.0)
            clip: true

            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                source: modelData.type === "image" ? root.mediaUrl(modelData.key) : ""
                cache: true
                asynchronous: true
                visible: modelData.type === "image"
            }

            Rectangle {
                anchors.fill: parent
                visible: modelData.type === "video"
                color: "#1d2430"
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#2d3b50" }
                    GradientStop { position: 1.0; color: "#121821" }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: 6

                    Label {
                        width: 80
                        text: "▶"
                        font.pixelSize: 30
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        width: 80
                        text: root.durationText(modelData.duration)
                        font.pixelSize: 11
                        color: "#d6e3f2"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (modelData.type === "image") {
                        root.imageRequested(modelData.key)
                    } else {
                        Qt.openUrlExternally(root.mediaUrl(modelData.key))
                    }
                }
            }
        }
    }
}
