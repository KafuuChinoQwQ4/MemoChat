pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

Popup {
    id: root
    modal: true
    focus: true
    // 只有点 X 或按 Escape 才关闭
    closePolicy: Popup.CloseOnEscape

    width: Math.min(480, (parent && parent.width > 0 ? parent.width : 700) - 48)
    height: Math.min(640, (parent && parent.height > 0 ? parent.height : 700) - 48)
    anchors.centerIn: parent

    property var backdrop: null
    property var controller: null
    property int currentUserUid: 0
    property int currentMomentId: 0
    property int momentUid: 0
    property string momentUserId: ""
    property string momentUserName: ""

    signal avatarProfileRequested(int uid, string name, string icon, string userId)

    // ── 纯白底背景 ──────────────────────────────────────
    background: Rectangle {
        color: "#ffffff"
        border.color: Qt.rgba(0.82, 0.84, 0.90, 0.6)
        border.width: 1
        radius: 14
        Rectangle {
            anchors.fill: parent; anchors.margins: -2; z: -1
            color: "transparent"
            border.color: Qt.rgba(0, 0, 0, 0.10); border.width: 1; radius: 16
        }
    }

    // 阻止事件穿透，防止点击内容时关闭弹窗
    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => mouse.accepted = false
    }

    // ── 整体垂直布局 ────────────────────────────────────
    Column {
        anchors.fill: parent
        spacing: 0

        // ── 标题栏: 固定 48px ─────────────────────────
        Rectangle {
            width: parent.width
            height: 48
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 8
                spacing: 8
                Label {
                    text: "朋友圈详情"
                    font.pixelSize: 15
                    font.weight: Font.Medium
                    color: "#1a1a1a"
                }
                Item { Layout.fillWidth: true }
                ToolButton {
                    icon.source: "qrc:/icons/close.png"
                    icon.width: 18
                    icon.height: 18
                    onClicked: root.close()
                }
            }

            // 标题栏底部细线
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Qt.rgba(0.88, 0.88, 0.92, 0.6)
            }
        }

        // ── 正文滚动区: 撑满中间空间 ───────────────────
        Flickable {
            id: flick
            width: parent.width
            height: parent.height - 48 - commentInputBar.height
            clip: true
            contentWidth: width
            contentHeight: contentCol.height + 16
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            // 内容列
            Column {
                id: contentCol
                width: flick.width - 32
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 12
                spacing: 10

                // ── 用户信息 ────────────────────────────
                RowLayout {
                    width: parent.width
                    spacing: 10
                    Rectangle {
                        width: 42; height: 42; radius: 10; clip: true
                        color: Qt.rgba(0.85, 0.88, 0.97, 1.0)
                        Image {
                            anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                            source: momentUserIcon; cache: true; asynchronous: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.avatarProfileRequested(root.momentUid,
                                                                   root.momentUserNick || root.momentUserName || "用户",
                                                                   root.momentUserIcon,
                                                                   root.momentUserId)
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 3
                        Label {
                            text: momentUserNick || " "
                            font.pixelSize: 14; font.weight: Font.Medium; color: "#1a1a1a"
                            wrapMode: Text.Wrap; Layout.fillWidth: true
                        }
                        Label {
                            text: momentLocation ? momentLocation : momentTimeText
                            font.pixelSize: 12; color: "#888888"
                            visible: text.length > 0; Layout.fillWidth: true
                        }
                    }
                }

                // ── 正文文字 ────────────────────────────
                Label {
                    width: parent.width; text: momentText
                    font.pixelSize: 15; color: "#1a1a1a"; wrapMode: Text.Wrap
                    visible: momentText.length > 0
                }

                // ── 图片网格 ────────────────────────────
                GridLayout {
                    width: parent.width; visible: momentMediaTiles.length > 0
                    columns: momentMediaTiles.length === 1 ? 1 : 3
                    rowSpacing: 6; columnSpacing: 6

                    Repeater {
                        model: momentMediaTiles
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: mediaTileHeight(modelData)
                            radius: 8; color: Qt.rgba(0.93, 0.95, 0.98, 1.0); clip: true
                            Image {
                                anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                                source: modelData.type === "image" ? imgUrl(modelData.key) : ""
                                cache: true; asynchronous: true
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
                                        text: "▶"
                                        font.pixelSize: 30
                                        color: "#ffffff"
                                        horizontalAlignment: Text.AlignHCenter
                                        width: 80
                                    }
                                    Label {
                                        text: videoDuration(modelData.duration)
                                        font.pixelSize: 11
                                        color: "#d6e3f2"
                                        horizontalAlignment: Text.AlignHCenter
                                        width: 80
                                    }
                                }
                            }
                            MouseArea {
                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (modelData.type === "image") {
                                        imgPopupLoader.active = true
                                        if (imgPopupLoader.item) imgPopupLoader.item.open(modelData.key)
                                    } else {
                                        Qt.openUrlExternally(imgUrl(modelData.key))
                                    }
                                }
                            }
                        }
                    }
                }

                // ── 点赞 ────────────────────────────────
                Row {
                    width: parent.width; spacing: 6; visible: likeCount > 0
                    Image {
                        width: 16; height: 16
                        source: "qrc:/icons/like_active.png"
                        fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Label {
                        text: likeCount + " 人觉得很赞"
                        font.pixelSize: 13; color: "#e84141"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // ── 分隔线 ──────────────────────────────
                Rectangle {
                    width: parent.width; height: 1
                    color: Qt.rgba(0.88, 0.88, 0.92, 0.5)
                }

                // ── 评论区白底卡片 ──────────────────────
                Rectangle {
                    width: parent.width
                    height: Math.max(64, commentColumn.implicitHeight + 20)
                    color: "#ffffff"
                    border.color: Qt.rgba(0.82, 0.84, 0.90, 0.5)
                    border.width: 1
                    radius: 10

                    Column {
                        id: commentColumn
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        // 标题行
                        Row {
                            spacing: 4
                            Label {
                                text: "评论"
                                font.pixelSize: 13; font.weight: Font.Medium; color: "#1a1a1a"
                            }
                            Label {
                                text: "(" + commentCount + ")"
                                font.pixelSize: 13; color: "#888888"
                                visible: (commentCount !== undefined && commentCount > 0)
                            }
                        }

                        // 无评论
                        Label {
                            text: "暂无评论，快来抢沙发"
                            font.pixelSize: 12; color: "#bbbbbb"
                            visible: !commentsLoading && (!commentList || commentList.length === 0)
                        }

                        // 加载中（count>0 但 list 为空）
                        Label {
                            text: "评论加载中..."
                            font.pixelSize: 12; color: "#999999"
                            visible: commentsLoading && (!commentList || commentList.length === 0)
                        }

                        // 评论列表
                        Column {
                            width: parent.width
                            spacing: 8
                            visible: commentListModel.count > 0
                            Repeater {
                                model: commentListModel
                                delegate: commentItem
                            }
                        }
                    }
                }
            }
        }

        // ── 底部输入框: 固定高度 56px ──────────────────
        // 底部左右下角圆角跟外层背景一致，融入而不溢出
        Rectangle {
            id: commentInputBar
            width: parent.width
            height: replyUid > 0 ? 92 : 64
            color: "#ffffff"
            clip: true   // 裁掉超出圆角的部分

            // 输入框顶部分隔线
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Qt.rgba(0.88, 0.88, 0.92, 0.5)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    visible: replyUid > 0
                    radius: 13
                    color: Qt.rgba(0.16, 0.48, 0.89, 0.08)
                    border.color: Qt.rgba(0.16, 0.48, 0.89, 0.18)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 4
                        spacing: 6

                        Label {
                            Layout.fillWidth: true
                            text: "回复 " + (replyNick || "用户")
                            color: "#2a7ae2"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        ToolButton {
                            id: clearReplyButton
                            implicitWidth: 22
                            implicitHeight: 22
                            padding: 0
                            text: "×"
                            font.pixelSize: 13
                            background: Rectangle {
                                radius: 11
                                color: clearReplyButton.hovered ? Qt.rgba(0.16, 0.24, 0.36, 0.08) : "transparent"
                            }
                            onClicked: {
                                replyUid = 0
                                replyNick = ""
                                commentInput.placeholderText = "写评论..."
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    TextField {
                        id: commentInput
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        placeholderText: "写评论..."
                        placeholderTextColor: "#aaaaaa"
                        font.pixelSize: 14
                        color: "#1a1a1a"
                        background: Rectangle {
                            color: "#f5f6f9"
                            radius: 12
                            border.color: Qt.rgba(0.82, 0.84, 0.90, 0.4)
                            border.width: 1
                        }
                        leftPadding: 12; rightPadding: 12
                        verticalAlignment: TextInput.AlignVCenter
                        onAccepted: sendComment()
                    }

                    Button {
                        id: sendBtn
                        Layout.preferredWidth: 64
                        Layout.fillHeight: true
                        enabled: commentInput.text.trim().length > 0 && !commentSending
                        background: Rectangle {
                            radius: 12
                            color: sendBtn.enabled ? "#2a7ae2" : Qt.rgba(0.82, 0.85, 0.90, 1.0)
                        }
                        contentItem: Label {
                            anchors.centerIn: parent
                            text: commentSending ? "..." : "发送"
                            color: sendBtn.enabled ? "#ffffff" : "#aaaaaa"
                            font.pixelSize: 14; font.weight: Font.Medium
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: sendComment()
                    }
                }
            }
        }
    }

    // ── 单条评论条目 ─────────────────────────────────────
    Component {
        id: commentItem
        Rectangle {
            id: commentRoot
            width: parent ? parent.width : 0
            height: commentCol.implicitHeight + 16
            color: Qt.rgba(0.96, 0.98, 1.0, 0.92)
            radius: 10
            border.color: Qt.rgba(0.82, 0.86, 0.92, 0.45)

            required property int commentId
            required property int commentUid
            required property string commentNick
            required property string commentText
            required property int targetReplyUid
            required property string targetReplyNick
            required property bool likedByMe
            required property int commentLikeCount
            required property string commentLikeText
            property bool ownComment: root.currentUserUid <= 0 || commentUid === Number(root.currentUserUid)

            Column {
                id: commentCol
                width: parent.width - 16
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: 8
                spacing: 6

                RowLayout {
                    width: parent.width
                    height: 24
                    spacing: 4
                    Label {
                        text: commentRoot.commentNick
                        font.pixelSize: 12; font.weight: Font.Medium; color: "#2a7ae2"
                        Layout.maximumWidth: parent.width * 0.42
                        elide: Text.ElideRight
                    }
                    Label {
                        text: commentRoot.targetReplyUid > 0 ? (" 回复 " + commentRoot.targetReplyNick) : ""
                        font.pixelSize: 12; color: "#2a7ae2"
                        visible: commentRoot.targetReplyUid > 0
                        Layout.maximumWidth: parent.width * 0.32
                        elide: Text.ElideRight
                    }
                    Label { text: "："; font.pixelSize: 12; color: "#555555" }
                    Item { Layout.fillWidth: true }
                    ToolButton {
                        id: commentMoreButton
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 24
                        text: "..."
                        padding: 0
                        font.pixelSize: 14
                        ToolTip.visible: hovered
                        ToolTip.delay: 120
                        ToolTip.text: "更多"
                        background: Rectangle {
                            radius: 9
                            color: commentMoreButton.down ? Qt.rgba(0.16, 0.24, 0.36, 0.12)
                                  : commentMoreButton.hovered ? Qt.rgba(0.16, 0.24, 0.36, 0.08)
                                  : "transparent"
                            border.width: commentMoreButton.hovered ? 1 : 0
                            border.color: Qt.rgba(0.16, 0.24, 0.36, 0.12)
                        }
                        onClicked: commentMenu.open()
                    }
                }
                Label {
                    width: parent.width
                    text: commentRoot.commentText.length > 0 ? commentRoot.commentText : " "
                    font.pixelSize: 13; color: "#1a1a1a"; wrapMode: Text.Wrap
                }

                Row {
                    width: parent.width
                    height: commentRoot.commentLikeCount > 0 ? 22 : 0
                    visible: height > 0
                    spacing: 6

                    Rectangle {
                        width: Math.min(parent.width, likedLabel.implicitWidth + 38)
                        height: 22
                        radius: 11
                        color: Qt.rgba(0.91, 0.20, 0.20, 0.07)
                        border.color: Qt.rgba(0.91, 0.20, 0.20, 0.14)

                        Image {
                            id: commentLikeIcon
                            width: 13
                            height: 13
                            source: "qrc:/icons/like_active.png"
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            fillMode: Image.PreserveAspectFit
                        }

                        Label {
                            id: likedLabel
                            anchors.left: commentLikeIcon.right
                            anchors.leftMargin: 5
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: commentRoot.commentLikeText.length > 0
                                  ? commentRoot.commentLikeText
                                  : (commentRoot.commentLikeCount + " 人点赞")
                            color: "#d64545"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Menu {
                id: commentMenu
                y: commentMoreButton.y + commentMoreButton.height + 4
                x: Math.max(0, parent.width - width - 8)
                width: 132
                background: Rectangle {
                    color: "#ffffff"
                    radius: 10
                    border.color: Qt.rgba(0.82, 0.84, 0.90, 0.75)
                }
                MenuItem {
                    id: replyCommentAction
                    height: 36
                    text: "回复"
                    contentItem: Label {
                        text: replyCommentAction.text
                        color: "#26384d"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: replyCommentAction.hovered ? Qt.rgba(0.16, 0.24, 0.36, 0.08) : "transparent"
                    }
                    onTriggered: root.prepareReply(commentRoot.commentUid, commentRoot.commentNick)
                }
                MenuItem {
                    id: likeCommentAction
                    height: 36
                    text: commentRoot.likedByMe ? "取消点赞" : "点赞评论"
                    contentItem: Label {
                        text: likeCommentAction.text
                        color: "#26384d"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: likeCommentAction.hovered ? Qt.rgba(0.16, 0.24, 0.36, 0.08) : "transparent"
                    }
                    onTriggered: root.toggleCommentLike(commentRoot.commentId, !commentRoot.likedByMe)
                }
                MenuSeparator {
                    visible: commentRoot.ownComment
                }
                MenuItem {
                    id: deleteCommentAction
                    height: 36
                    text: "删除"
                    visible: commentRoot.ownComment
                    enabled: visible
                    contentItem: Label {
                        text: deleteCommentAction.text
                        color: "#d64545"
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 8
                        color: deleteCommentAction.hovered ? Qt.rgba(0.86, 0.18, 0.18, 0.08) : "transparent"
                    }
                    onTriggered: root.deleteOwnComment(commentRoot.commentId)
                }
            }
        }
    }

    // ── Data ─────────────────────────────────────────────
    property string momentUserNick: ""
    property string momentUserIcon: "qrc:/res/head_1.jpg"
    property string momentLocation: ""
    property string momentTimeText: ""
    property string momentText: ""
    property var momentMediaTiles: []
    property int likeCount: 0
    property var likeNames: []
    property int commentCount: 0
    property var commentList: []
    property int replyUid: 0
    property string replyNick: ""
    property bool commentSending: false
    property bool commentsLoading: false

    ListModel {
        id: commentListModel
    }

    // 评论卡片高度: 有内容就撑开，最小64px
    property int commentCardHeight: {
        if (!commentList || commentList.length === 0) return 62
        var total = 22  // 标题
        for (var i = 0; i < commentList.length; i++) {
            total += 64
            if (i > 0) total += 8
        }
        return total + 10
    }

    // ── Helpers ─────────────────────────────────────────
    function timeAgo(ts) {
        if (!ts) return ""
        var nowSec = Math.floor(Date.now() / 1000)
        var diff = nowSec - Math.floor(ts / 1000)
        if (diff < 60) return "刚刚"
        if (diff < 3600) return Math.floor(diff / 60) + "分钟前"
        if (diff < 86400) return Math.floor(diff / 3600) + "小时前"
        if (diff < 604800) return Math.floor(diff / 86400) + "天前"
        return new Date(ts).toLocaleDateString()
    }

    function imgHeight(item) {
        var w = item.width || 200, h = item.height || 200
        return Math.min(160, Math.max(72, 120 * (h / (w || 1))))
    }

    function mediaTileHeight(item) {
        if (item.type === "video")
            return 124
        return imgHeight(item)
    }

    function videoDuration(durationMs) {
        if (!durationMs || durationMs <= 0)
            return "点击打开视频"
        var totalSec = Math.floor(durationMs / 1000)
        var minutes = Math.floor(totalSec / 60)
        var seconds = totalSec % 60
        return minutes + ":" + (seconds < 10 ? "0" + seconds : seconds)
    }

    function imgUrl(key) {
        return key ? (gate_url_prefix + "/media/download?asset=" + key) : ""
    }

    function applySnapshot() {
        if (!controller || !controller.model || currentMomentId <= 0) {
            console.log("[MomentsDetail] applySnapshot: skipped, controller=" + !!controller + " model=" + (controller ? !!controller.model : false) + " momentId=" + currentMomentId)
            return
        }
        var snap = controller.model.snapshotMoment(currentMomentId)
        if (!snap || snap.momentId === undefined) {
            console.log("[MomentsDetail] applySnapshot: snap not found for momentId=" + currentMomentId)
            return
        }

        console.log("[MomentsDetail] applySnapshot: got snap, commentCount=" + snap.commentCount + " comments=" + (snap.comments ? snap.comments.length : 0))
        momentUid = snap.uid || 0
        momentUserId = snap.userId || ""
        momentUserName = snap.userName || ""
        momentUserNick = snap.userNick || ""
        momentUserIcon = (snap.userIcon && snap.userIcon.length) ? snap.userIcon : "qrc:/res/head_1.jpg"
        momentLocation = snap.location || ""
        momentTimeText = timeAgo(snap.createdAt)
        likeCount = snap.likeCount !== undefined ? snap.likeCount : 0
        commentCount = snap.commentCount !== undefined ? snap.commentCount : 0

        var items = snap.items || [], textParts = [], mediaTiles = []
        for (var i = 0; i < items.length; i++) {
            var it = items[i]
            var type = String(it.media_type || it.mediaType || it.type || "").toLowerCase()
            var content = String(it.content || "")
            var key = String(it.media_key || it.mediaKey || it.key || "")
            if ((type === "image" || type === "video") && key.length > 0) {
                if (type === "image") {
                    mediaTiles.push({ type: "image", key: key, width: it.width || 0, height: it.height || 0 })
                } else {
                    mediaTiles.push({ type: "video", key: key, duration: it.duration_ms || it.durationMs || 0 })
                }
            } else if (content.length > 0) {
                textParts.push(content)
            }
        }
        momentText = textParts.join("\n")
        momentMediaTiles = mediaTiles

        var rawLikes = snap.likes || [], names = []
        for (var j = 0; j < rawLikes.length; j++) {
            if (rawLikes[j].user_nick) names.push(rawLikes[j].user_nick)
        }
        likeNames = names
        var rawComments = snap.comments || []
        var normalizedComments = []
        commentListModel.clear()
        for (var c = 0; c < rawComments.length; c++) {
            var cm = rawComments[c]
            var normalizedComment = {
                id: Number(cm.id || cm.comment_id || 0),
                uid: Number(cm.uid || 0),
                user_nick: String(cm.user_nick || cm.userNick || "用户"),
                user_icon: String(cm.user_icon || cm.userIcon || ""),
                content: String(cm.content || cm.text || cm.comment || ""),
                reply_uid: Number(cm.reply_uid || cm.replyUid || 0),
                reply_nick: String(cm.reply_nick || cm.replyNick || "用户"),
                created_at: cm.created_at || cm.createdAt || 0,
                has_liked: !!(cm.has_liked || cm.hasLiked),
                like_count: Number(cm.like_count || cm.likeCount || 0),
                likes: cm.likes || []
            }
            var commentLikeNames = []
            for (var li = 0; li < normalizedComment.likes.length; li++) {
                var likeItem = normalizedComment.likes[li]
                var likeName = String(likeItem.user_nick || likeItem.userNick || "")
                if (likeName.length > 0)
                    commentLikeNames.push(likeName)
            }
            if (normalizedComment.like_count <= 0)
                normalizedComment.like_count = commentLikeNames.length
            normalizedComments.push(normalizedComment)
            commentListModel.append({
                commentId: normalizedComment.id,
                commentUid: normalizedComment.uid,
                commentNick: normalizedComment.user_nick,
                commentText: normalizedComment.content,
                targetReplyUid: normalizedComment.reply_uid,
                targetReplyNick: normalizedComment.reply_nick,
                likedByMe: normalizedComment.has_liked,
                commentLikeCount: normalizedComment.like_count,
                commentLikeText: commentLikeNames.join("、")
            })
        }
        commentList = normalizedComments
    }

    function clearSnapshot() {
        momentUid = 0
        momentUserId = ""
        momentUserName = ""
        momentUserNick = ""
        momentUserIcon = "qrc:/res/head_1.jpg"
        momentLocation = ""
        momentTimeText = ""
        momentText = ""
        momentMediaTiles = []
        likeCount = 0
        likeNames = []
        commentCount = 0
        commentList = []
        commentListModel.clear()
    }

    function openMoment(momentId) {
        console.log("[MomentsDetail] openMoment called, momentId=" + momentId + " controller=" + !!root.controller)
        root.currentMomentId = momentId
        clearSnapshot()
        replyUid = 0
        replyNick = ""
        commentInput.placeholderText = "写评论..."
        commentsLoading = true
        applySnapshot()
        root.open()
        console.log("[MomentsDetail] openMoment: calling refreshMoment")
        if (root.controller) {
            root.controller.refreshMoment(momentId)
            root.controller.refreshComments(momentId)
        } else {
            console.log("[MomentsDetail] openMoment: NO CONTROLLER!")
        }
    }

    function sendComment() {
        var text = commentInput.text.trim()
        if (!text || !root.controller || root.currentMomentId <= 0) return
        commentSending = true
        commentsLoading = true
        root.controller.addComment(root.currentMomentId, text, replyUid)
        commentInput.clear()
        replyUid = 0; replyNick = ""
        commentInput.placeholderText = "写评论..."
    }

    function prepareReply(uid, nick) {
        replyUid = uid || 0
        replyNick = nick || "用户"
        commentInput.placeholderText = replyUid > 0 ? ("回复 " + replyNick + "...") : "写评论..."
        commentInput.forceActiveFocus()
    }

    function toggleCommentLike(commentId, liked) {
        if (!commentId || commentId <= 0 || !root.controller || root.currentMomentId <= 0)
            return
        root.controller.toggleCommentLike(root.currentMomentId, commentId, liked)
    }

    function deleteOwnComment(commentId) {
        if (!root.controller || root.currentMomentId <= 0 || commentId <= 0)
            return
        commentsLoading = true
        root.controller.deleteComment(root.currentMomentId, commentId)
    }

    Connections {
        target: controller
        function onCommentsLoadingChanged(momentId, loading) {
            if (root.opened && momentId === root.currentMomentId) {
                commentsLoading = loading
            }
        }
        function onCommentAdded(momentId) {
            if (root.opened && (momentId <= 0 || momentId === root.currentMomentId)) {
                commentSending = false
                if (replyUid === 0)
                    commentInput.placeholderText = "写评论..."
                applySnapshot()
            }
        }
        function onMomentRefreshed(momentId) {
            console.log("[MomentsDetail] onMomentRefreshed received, momentId=" + momentId)
            if (root.opened && momentId === root.currentMomentId) {
                commentSending = false
                applySnapshot()
            }
        }
    }

    Loader {
        id: imgPopupLoader
        active: false
        sourceComponent: Component {
            Popup {
                id: imgPopup
                modal: true; focus: true
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                property string currentKey: ""
                function open(key) { currentKey = key; imgPopup.open() }
                width: parent && parent.width ? parent.width - 40 : 680
                height: parent && parent.height ? parent.height - 40 : 500
                anchors.centerIn: Overlay.overlay
                background: Rectangle { color: "#111111"; anchors.fill: parent }
                Image {
                    anchors.fill: parent; fillMode: Image.PreserveAspectFit
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
