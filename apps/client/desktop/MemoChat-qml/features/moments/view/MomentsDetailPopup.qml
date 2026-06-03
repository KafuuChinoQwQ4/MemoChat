pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "qrc:/qml/components"

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

        MomentsDetailHeader {
            width: parent.width
            height: 48
            onCloseRequested: root.close()
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

                MomentsDetailAuthorRow {
                    width: parent.width
                    userNick: root.momentUserNick
                    userName: root.momentUserName
                    userIcon: root.momentUserIcon
                    locationText: root.momentLocation ? root.momentLocation : root.momentTimeText
                    uid: root.momentUid
                    userId: root.momentUserId
                    onAvatarProfileRequested: function(uid, name, icon, userId) {
                        root.avatarProfileRequested(uid, name, icon, userId)
                    }
                }

                // ── 正文文字 ────────────────────────────
                Label {
                    width: parent.width; text: momentText
                    font.pixelSize: 15; color: "#1a1a1a"; wrapMode: Text.Wrap
                    visible: momentText.length > 0
                }

                MomentsDetailMediaGrid {
                    width: parent.width
                    mediaTiles: root.momentMediaTiles
                    mediaUrlResolver: root.imgUrl
                    mediaHeightResolver: root.mediaTileHeight
                    videoDurationFormatter: root.videoDuration
                    onImageRequested: function(key) {
                        imagePreviewPopup.openImage(key)
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

                MomentsCommentList {
                    width: parent.width
                    commentModel: commentListModel
                    commentsLoading: root.commentsLoading
                    commentCount: root.commentCount
                    currentUserUid: root.currentUserUid
                    onReplyRequested: function(uid, nick) { root.prepareReply(uid, nick) }
                    onLikeToggled: function(commentId, liked) { root.toggleCommentLike(commentId, liked) }
                    onDeleteRequested: function(commentId) { root.deleteOwnComment(commentId) }
                }
            }
        }

        MomentsCommentComposer {
            id: commentInputBar
            width: parent.width
            replyUid: root.replyUid
            replyNick: root.replyNick
            commentSending: root.commentSending
            placeholderText: root.commentPlaceholderText
            onReplyCleared: root.clearReplyTarget()
            onCommentSubmitted: function(text) { root.sendComment(text) }
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
    property string commentPlaceholderText: "写评论..."
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
        return key ? (gateMediaUrlPrefix + "/media/download?asset=" + key) : ""
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
        clearReplyTarget()
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

    function sendComment(text) {
        text = String(text || "").trim()
        if (!text || !root.controller || root.currentMomentId <= 0) return
        commentSending = true
        commentsLoading = true
        root.controller.addComment(root.currentMomentId, text, replyUid)
        commentInputBar.clearInput()
        clearReplyTarget()
    }

    function clearReplyTarget() {
        replyUid = 0
        replyNick = ""
        commentPlaceholderText = "写评论..."
    }

    function prepareReply(uid, nick) {
        replyUid = uid || 0
        replyNick = nick || "用户"
        commentPlaceholderText = replyUid > 0 ? ("回复 " + replyNick + "...") : "写评论..."
        commentInputBar.focusInput()
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
                    commentPlaceholderText = "写评论..."
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

    MomentsImagePreviewPopup {
        id: imagePreviewPopup
        gatewayPrefix: gateMediaUrlPrefix
    }
}
