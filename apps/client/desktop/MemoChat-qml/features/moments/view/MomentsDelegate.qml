pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "qrc:/qml/components"

Rectangle {
    id: root
    color: "transparent"
    clip: true
    // Loader 未设 height：用内容固有高度 + 上下边距，避免正文与点赞行重叠
    implicitHeight: Math.max(contentLayout.implicitHeight, contentLayout.childrenRect.height) + 24
    height: implicitHeight

    property var backdrop: null
    property var controller: null
    property var momentData: null
    property int momentId: momentData ? momentData.momentId : 0
    property int uid: momentData ? momentData.uid : 0
    property string userName: momentData ? momentData.userName : ""
    property string userNick: momentData ? momentData.userNick : ""
    property string userIcon: momentData ? momentData.userIcon : "qrc:/res/head_1.jpg"
    property string location: momentData ? momentData.location : ""
    property var createdAt: momentData ? momentData.createdAt : 0
    property int likeCount: momentData ? momentData.likeCount : 0
    property int commentCount: momentData ? momentData.commentCount : 0
    property bool hasLiked: momentData ? momentData.hasLiked : false
    property var items: momentData ? momentData.items : []
    property string listTextContent: ""
    property string textContent: listTextContent.length > 0 ? listTextContent
                                 : (momentData && momentData.textContent ? String(momentData.textContent) : buildTextContent(items))
    property bool hasMediaContent: containsMediaContent(items)
    property bool hasRenderableContent: textContent.length > 0 || hasMediaContent
    property bool canDelete: false

    signal likeClicked()
    signal commentClicked()
    signal avatarClicked()
    signal deleteClicked()

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 12
        blurRadius: 16
        fillColor: Qt.rgba(1, 1, 1, 0.88)
        strokeColor: Qt.rgba(0.85, 0.85, 0.90, 0.5)
    }

    ColumnLayout {
        id: contentLayout
        width: parent.width - 24
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 12
        spacing: 10

        MomentsHeader {
            Layout.fillWidth: true
            userName: root.userName
            userNick: root.userNick
            userIcon: root.userIcon
            locationText: root.location ? root.location : root.timeAgoText(root.createdAt)
            canDelete: root.canDelete
            onAvatarClicked: root.avatarClicked()
            onDeleteClicked: root.deleteClicked()
        }

        // Content items (images / text) — Layout.* 仅对 Layout 容器的直接子项生效，故媒体块用真实容器汇总高度
        Label {
            Layout.fillWidth: true
            visible: !root.hasRenderableContent
            text: "内容未保存，请重新发布"
            font.pixelSize: 14
            color: "#8a93a3"
            wrapMode: Text.Wrap
        }

        Label {
            Layout.fillWidth: true
            visible: root.textContent.length > 0
            text: root.textContent
            font.pixelSize: 15
            color: "#1a1a1a"
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignLeft
        }

        Item {
            id: mediaColumn
            Layout.fillWidth: true
            Layout.preferredHeight: root.mediaContentHeight(root.items)
            Layout.minimumHeight: root.mediaContentHeight(root.items)
            visible: root.hasMediaContent
            implicitHeight: root.mediaContentHeight(root.items)
            clip: true

            Column {
                id: mediaStack
                width: parent.width
                spacing: 8

                Repeater {
                    model: root.items
                    delegate: Item {
                        id: blockRoot
                        required property var modelData
                        width: mediaStack.width
                        implicitHeight: root.mediaItemHeight(blockRoot.modelData)
                        height: visible ? root.mediaItemHeight(blockRoot.modelData) : 0
                        visible: root.isMediaItem(blockRoot.modelData) && root.itemMediaKey(blockRoot.modelData).length > 0

                        Column {
                            id: blockColumn
                            width: parent.width
                            spacing: 8

                            Rectangle {
                                id: imageBlock
                                visible: root.itemType(blockRoot.modelData) === "image"
                                width: Math.min(blockRoot.width, root.imageMaxDim(blockRoot.modelData))
                                height: visible ? blockRoot.height : 0
                                color: Qt.rgba(0.92, 0.94, 0.97, 1.0)
                                radius: 8
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    fillMode: Image.PreserveAspectCrop
                                    source: root.mediaUrl(root.itemMediaKey(blockRoot.modelData))
                                    cache: true
                                    asynchronous: true
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        imageViewerPopup.showImage(root.itemMediaKey(blockRoot.modelData))
                                    }
                                }
                            }

                            Rectangle {
                                id: videoBlock
                                visible: root.itemType(blockRoot.modelData) === "video"
                                width: Math.min(blockRoot.width, 320)
                                height: visible ? blockRoot.height : 0
                                radius: 10
                                clip: true
                                color: "#202834"
                                border.color: Qt.rgba(0.18, 0.22, 0.28, 0.85)

                                Rectangle {
                                    anchors.fill: parent
                                    gradient: Gradient {
                                        GradientStop { position: 0.0; color: "#324257" }
                                        GradientStop { position: 1.0; color: "#131922" }
                                    }
                                }

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 8

                                    Label {
                                        text: "▶"
                                        font.pixelSize: 34
                                        color: "#ffffff"
                                        horizontalAlignment: Text.AlignHCenter
                                        width: 120
                                    }

                                    Label {
                                        text: root.videoDurationText(blockRoot.modelData.duration_ms)
                                        font.pixelSize: 12
                                        color: "#dbe7f6"
                                        horizontalAlignment: Text.AlignHCenter
                                        width: 120
                                    }
                                }

                                Label {
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    anchors.margins: 12
                                    text: "视频内容"
                                    font.pixelSize: 12
                                    color: "#dbe5f2"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Qt.openUrlExternally(root.mediaUrl(root.itemMediaKey(blockRoot.modelData)))
                                }
                            }
                        }
                    }
                }
            }
        }

        MomentsActionBar {
            Layout.fillWidth: true
            Layout.topMargin: 10
            likeCount: root.likeCount
            commentCount: root.commentCount
            hasLiked: root.hasLiked
            onLikeClicked: root.likeClicked()
            onCommentClicked: root.commentClicked()
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
        return root.controller ? root.controller.mediaUrlForKey(key) : ""
    }

    function itemType(item) {
        return String((item && (item.media_type || item.mediaType || item.type)) || "").toLowerCase()
    }

    function itemMediaKey(item) {
        return String((item && (item.media_key || item.mediaKey || item.key)) || "")
    }

    function itemContent(item) {
        return String((item && item.content) || "")
    }

    function buildTextContent(items) {
        if (!items || items.length === 0)
            return ""
        var parts = []
        for (var i = 0; i < items.length; i++) {
            var it = items[i]
            var content = itemContent(it)
            if (content.length > 0 && (!isMediaItem(it) || !itemMediaKey(it))) {
                parts.push(content)
            }
        }
        return parts.join("\n")
    }

    function containsMediaContent(items) {
        if (!items || items.length === 0)
            return false
        for (var i = 0; i < items.length; i++) {
            var it = items[i]
            if (isMediaItem(it) && itemMediaKey(it).length > 0) {
                return true
            }
        }
        return false
    }

    function isMediaItem(item) {
        var type = itemType(item)
        return type === "image" || type === "video"
    }

    function mediaItemHeight(item) {
        if (!isMediaItem(item) || itemMediaKey(item).length === 0)
            return 0
        if (itemType(item) === "video")
            return 188
        return imageHeight(item)
    }

    function mediaContentHeight(items) {
        if (!items || items.length === 0)
            return 0
        var total = 0
        var count = 0
        for (var i = 0; i < items.length; i++) {
            var itemHeight = mediaItemHeight(items[i])
            if (itemHeight <= 0)
                continue
            if (count > 0)
                total += 8
            total += itemHeight
            count += 1
        }
        return total
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

    function videoDurationText(durationMs) {
        if (!durationMs || durationMs <= 0)
            return "点击打开视频"
        var totalSec = Math.floor(durationMs / 1000)
        var minutes = Math.floor(totalSec / 60)
        var seconds = totalSec % 60
        return minutes + ":" + (seconds < 10 ? "0" + seconds : seconds)
    }

    Popup {
        id: imageViewerPopup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: Math.min(root.Window ? root.Window.width : 800, 720)
        height: Math.min(root.Window ? root.Window.height : 600, 540)
        anchors.centerIn: Overlay.overlay

        property string currentKey: ""

        function showImage(key) {
            currentKey = key
            open()
        }

        background: Rectangle { color: "#111111"; anchors.fill: parent }
        Image {
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            source: root.mediaUrl(imageViewerPopup.currentKey)
            cache: false
        }
        MouseArea {
            anchors.fill: parent
            onClicked: imageViewerPopup.close()
        }
    }
}
