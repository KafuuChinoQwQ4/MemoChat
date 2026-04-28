pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

Rectangle {
    id: root
    color: "transparent"
    // Loader 未设 height：用内容固有高度 + 上下边距，避免正文与点赞行重叠
    implicitHeight: contentLayout.implicitHeight + 24

    property var backdrop: null
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
                    text: root.userNick || root.userName || "用户"
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

            ToolButton {
                id: moreButton
                visible: root.canDelete
                implicitWidth: 32
                implicitHeight: 32
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.delay: 120
                ToolTip.text: "更多"
                background: Rectangle {
                    radius: 10
                    color: moreButton.down ? Qt.rgba(0.16, 0.24, 0.36, 0.12)
                          : moreButton.hovered ? Qt.rgba(0.16, 0.24, 0.36, 0.08)
                          : "transparent"
                    border.width: moreButton.hovered ? 1 : 0
                    border.color: Qt.rgba(0.16, 0.24, 0.36, 0.12)
                }
                contentItem: Label {
                    text: "..."
                    color: "#4d5f73"
                    font.pixelSize: 16
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: postMenu.open()

                Menu {
                    id: postMenu
                    y: moreButton.height + 4
                    width: 116
                    background: Rectangle {
                        color: "#ffffff"
                        radius: 10
                        border.color: Qt.rgba(0.82, 0.84, 0.90, 0.75)
                        layer.enabled: true
                    }
                    MenuItem {
                        id: postDeleteAction
                        height: 38
                        text: "删除"
                        contentItem: Label {
                            text: postDeleteAction.text
                            color: "#d64545"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 8
                            color: postDeleteAction.hovered ? Qt.rgba(0.86, 0.18, 0.18, 0.08) : "transparent"
                        }
                        onTriggered: root.deleteClicked()
                    }
                }
            }
        }

        // Content items (images / text) — Layout.* 仅对 Layout 容器的直接子项生效，故用 Item + 显式宽高
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

        Repeater {
            model: root.items
            delegate: Item {
                id: blockRoot
                Layout.fillWidth: true
                Layout.preferredHeight: blockColumn.implicitHeight

                Column {
                    id: blockColumn
                    width: parent.width
                    spacing: 8

                    Rectangle {
                        id: imageBlock
                        visible: itemType(modelData) === "image" && itemMediaKey(modelData).length > 0
                        width: Math.min(blockRoot.width, imageMaxDim(modelData))
                        height: visible ? imageHeight(modelData) : 0
                        color: Qt.rgba(0.92, 0.94, 0.97, 1.0)
                        radius: 8
                        clip: true

                        Image {
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectCrop
                            source: mediaUrl(itemMediaKey(modelData))
                            cache: true
                            asynchronous: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                imageViewerLoader.active = true
                                if (imageViewerLoader.item) {
                                    imageViewerLoader.item.open(itemMediaKey(modelData))
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: videoBlock
                        visible: itemType(modelData) === "video" && itemMediaKey(modelData).length > 0
                        width: Math.min(blockRoot.width, 320)
                        height: visible ? 188 : 0
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
                                text: videoDurationText(modelData.duration_ms)
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
                            onClicked: Qt.openUrlExternally(mediaUrl(itemMediaKey(modelData)))
                        }
                    }
                }
            }
        }

        // Action row: 独占一行、右对齐，与正文留出间距
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 10
            spacing: 0

            Item { Layout.fillWidth: true }

            RowLayout {
                spacing: 20

                MouseArea {
                    Layout.preferredWidth: Math.max(44, likeRow.implicitWidth + 16)
                    Layout.preferredHeight: 36
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.likeClicked()

                    Row {
                        id: likeRow
                        anchors.centerIn: parent
                        spacing: 4

                        Image {
                            width: 18
                            height: 18
                            source: root.hasLiked ? "qrc:/icons/like_active.png" : "qrc:/icons/like.png"
                            fillMode: Image.PreserveAspectFit
                        }
                        Label {
                            text: root.likeCount > 0 ? root.likeCount : ""
                            font.pixelSize: 13
                            color: root.hasLiked ? "#e84141" : "#555555"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                MouseArea {
                    Layout.preferredWidth: Math.max(44, commentActionRow.implicitWidth + 16)
                    Layout.preferredHeight: 36
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.commentClicked()

                    Row {
                        id: commentActionRow
                        anchors.centerIn: parent
                        spacing: 4

                        Image {
                            width: 18
                            height: 18
                            source: "qrc:/icons/comment.png"
                            fillMode: Image.PreserveAspectFit
                        }
                        Label {
                            text: root.commentCount > 0 ? root.commentCount : ""
                            font.pixelSize: 13
                            color: "#555555"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }

        // Comment preview (up to 2 comments)
        Column {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 2
            visible: root.commentCount > 0 && commentPreviewItems.length > 0

            property var commentPreviewItems: root.momentData && root.momentData.comments ? root.momentData.comments : []

            Repeater {
                model: Math.min(2, commentPreviewItems.length)
                delegate: Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: commentRow.implicitHeight + 2

                    RowLayout {
                        id: commentRow
                        anchors.fill: parent
                        spacing: 4
                        Label {
                            text: (commentPreviewItems[index].user_nick || "用户") + "："
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            color: "#2a7ae2"
                        }
                        Label {
                            text: commentPreviewItems[index].reply_uid && commentPreviewItems[index].reply_uid !== 0
                                  ? ("回复 " + (commentPreviewItems[index].reply_nick || "用户") + "：") : ""
                            font.pixelSize: 12
                            color: "#2a7ae2"
                            visible: commentPreviewItems[index].reply_uid && commentPreviewItems[index].reply_uid !== 0
                        }
                        Label {
                            text: commentPreviewItems[index].content || ""
                            font.pixelSize: 12
                            color: "#555555"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Label {
                visible: root.commentCount > 2
                text: "共 " + root.commentCount + " 条评论"
                font.pixelSize: 11
                color: "#888888"
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
                    fillMode: Image.PreserveAspectFit
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
