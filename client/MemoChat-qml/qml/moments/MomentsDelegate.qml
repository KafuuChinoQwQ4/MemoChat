import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

Rectangle {
    id: root
    color: "transparent"

    property var backdrop: null
    property var momentData: null
    property int momentId: momentData ? momentData.momentId : 0
    property int uid: momentData ? momentData.uid : 0
    property string userNick: momentData ? momentData.userNick : ""
    property string userIcon: momentData ? momentData.userIcon : "qrc:/res/head_1.jpg"
    property string location: momentData ? momentData.location : ""
    property var createdAt: momentData ? momentData.createdAt : 0
    property int likeCount: momentData ? momentData.likeCount : 0
    property int commentCount: momentData ? momentData.commentCount : 0
    property bool hasLiked: momentData ? momentData.hasLiked : false
    property var items: momentData ? momentData.items : []

    signal likeClicked()
    signal commentClicked()
    signal avatarClicked()

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 12
        blurRadius: 16
        fillColor: Qt.rgba(1, 1, 1, 0.88)
        strokeColor: Qt.rgba(0.85, 0.85, 0.90, 0.5)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // Header row: avatar + name + location
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                width: 42
                height: 42
                radius: 8
                clip: true
                color: Qt.rgba(0.8, 0.85, 0.95, 0.5)
                Image {
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectCrop
                    source: root.userIcon
                    cache: true
                    asynchronous: true
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.avatarClicked()
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    text: root.userNick
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: "#1a1a1a"
                }
                Label {
                    text: root.location ? root.location : timeAgoText(root.createdAt)
                    font.pixelSize: 12
                    color: "#888888"
                }
            }

            Item { Layout.fillWidth: true }
        }

        // Content items (images / text)
        Repeater {
            model: root.items
            delegate: Item {
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight

                property real contentHeight: {
                    var type = modelData.media_type
                    if (type === "text") {
                        return textItem.contentHeight
                    }
                    if (type === "image") {
                        return 180
                    }
                    return 200
                }

                // Text content
                Label {
                    id: textItem
                    property real contentHeight: visible ? implicitHeight : 0
                    visible: modelData.media_type === "text"
                    text: modelData.content || ""
                    font.pixelSize: 14
                    color: "#1a1a1a"
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                }

                // Image grid
                Rectangle {
                    visible: modelData.media_type === "image"
                    width: parent ? Math.min(parent.width, imageMaxDim(modelData)) : 200
                    height: visible ? imageHeight(modelData) : 0
                    color: Qt.rgba(0.92, 0.94, 0.97, 1.0)
                    radius: 8
                    clip: true

                    Image {
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectCrop
                        source: mediaUrl(modelData.media_key)
                        cache: true
                        asynchronous: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            imageViewerLoader.active = true
                            if (imageViewerLoader.item) {
                                imageViewerLoader.item.open(modelData.media_key)
                            }
                        }
                    }
                }
            }
        }

        // Action row: like + comment
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Item { Layout.fillWidth: true }

            // Like button
            RowLayout {
                spacing: 4
                MouseArea {
                    implicitWidth: likeLabel.width + 8
                    implicitHeight: 28
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.likeClicked()
                    Label {
                        id: likeLabel
                        anchors.centerIn: parent
                        text: (root.hasLiked ? "♥ " : "♡ ") + (root.likeCount > 0 ? root.likeCount : "")
                        font.pixelSize: 13
                        color: root.hasLiked ? "#e84141" : "#555555"
                    }
                }

                // Comment button
                MouseArea {
                    implicitWidth: commentLabel.width + 8
                    implicitHeight: 28
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.commentClicked()
                    Label {
                        id: commentLabel
                        anchors.centerIn: parent
                        text: "💬 " + (root.commentCount > 0 ? root.commentCount : "")
                        font.pixelSize: 13
                        color: "#555555"
                    }
                }
            }
        }

        // Comment preview (last 2 comments)
        Repeater {
            model: root.commentCount > 0 ? 2 : 0
            delegate: Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
            }
        }
    }

    // Helper functions
    function timeAgoText(ts) {
        if (!ts) return ""
        var nowSec = Math.floor(Date.now() / 1000)
        var diff = nowSec - Math.floor(ts / 1000)
        if (diff < 60) return "刚刚"
        if (diff < 3600) return Math.floor(diff / 60) + "分钟前"
        if (diff < 86400) return Math.floor(diff / 3600) + "小时前"
        if (diff < 604800) return Math.floor(diff / 86400) + "天前"
        return new Date(ts).toLocaleDateString()
    }

    function mediaUrl(key) {
        if (!key) return ""
        return gate_url_prefix + "/media/download?asset=" + key
    }

    function imageMaxDim(item) {
        var w = item.width || 200
        var h = item.height || 200
        var aspect = w / (h || 1)
        return Math.min(320, Math.max(80, 200 * aspect))
    }

    function imageHeight(item) {
        var w = item.width || 200
        var h = item.height || 200
        var aspect = h / (w || 1)
        return Math.min(240, Math.max(60, 200 * aspect))
    }

    // Image viewer popup
    Loader {
        id: imageViewerLoader
        active: false
        sourceComponent: Component {
            Popup {
                id: imgPopup
                modal: true
                focus: true
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                width: Math.min(root.Window ? root.Window.width : 800, 720)
                height: Math.min(root.Window ? root.Window.height : 600, 540)
                anchors.centerIn: Overlay.overlay

                property string currentKey: ""

                function open(key) {
                    currentKey = key
                    open()
                }

                background: Rectangle { color: "#111111"; anchors.fill: parent }
                Image {
                    anchors.fill: parent
                    fillMode: Image.Contain
                    source: gate_url_prefix + "/media/download?asset=" + imgPopup.currentKey
                    cache: false
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: imgPopup.close()
                }
            }
        }
    }
}
