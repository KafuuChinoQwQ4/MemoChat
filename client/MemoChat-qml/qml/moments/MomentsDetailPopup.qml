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
            height: parent.height - 48 - 56
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
                    height: Math.max(64, commentCardHeight)
                    color: "#ffffff"
                    border.color: Qt.rgba(0.82, 0.84, 0.90, 0.5)
                    border.width: 1
                    radius: 10

                    Column {
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
                            spacing: 8
                            visible: commentList && commentList.length > 0
                            Repeater {
                                model: commentList
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
            width: parent.width
            height: 56
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

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
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
                        radius: 6
                        border.color: Qt.rgba(0.82, 0.84, 0.90, 0.4)
                        border.width: 1
                    }
                    leftPadding: 12; rightPadding: 12
                    verticalAlignment: TextInput.AlignVCenter
                    onAccepted: sendComment()
                }

                Button {
                    id: sendBtn
                    Layout.preferredWidth: 60
                    Layout.fillHeight: true
                    enabled: commentInput.text.trim().length > 0 && !commentSending
                    background: Rectangle {
                        radius: 6
                        color: sendBtn.enabled ? "#2a7ae2" : Qt.rgba(0.82, 0.85, 0.90, 1.0)
                    }
                    contentItem: Label {
                        anchors.centerIn: parent
                        text: commentSending ? "..." : "发送"
                        color: sendBtn.enabled ? "#ffffff" : "#aaaaaa"
                        font.pixelSize: 14; font.weight: Font.Medium
                    }
                    onClicked: sendComment()
                }
            }
        }
    }

    // ── 单条评论条目 ─────────────────────────────────────
    Component {
        id: commentItem
        Rectangle {
            width: parent ? parent.width : 0
            height: commentCol.height + 12
            color: Qt.rgba(0.93, 0.95, 1.0, 0.55)
            radius: 6

            Column {
                id: commentCol
                anchors.fill: parent
                anchors.margins: 8
                spacing: 3

                RowLayout {
                    anchors.fill: parent
                    spacing: 3
                    Label {
                        text: modelData.user_nick || "用户"
                        font.pixelSize: 12; font.weight: Font.Medium; color: "#2a7ae2"
                    }
                    Label {
                        text: modelData.reply_uid && modelData.reply_uid !== 0
                              ? (" 回复 " + (modelData.reply_nick || "用户")) : ""
                        font.pixelSize: 12; color: "#2a7ae2"
                        visible: modelData.reply_uid && modelData.reply_uid !== 0
                    }
                    Label { text: "："; font.pixelSize: 12; color: "#555555" }
                }
                Label {
                    anchors.fill: parent
                    text: modelData.content || ""
                    font.pixelSize: 13; color: "#1a1a1a"; wrapMode: Text.Wrap
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

    // 评论卡片高度: 有内容就撑开，最小64px
    property int commentCardHeight: {
        if (!commentList || commentList.length === 0) return 62
        var total = 22  // 标题
        for (var i = 0; i < commentList.length; i++) {
            total += 50
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
            if (it.media_type === "text") { if (it.content) textParts.push(it.content) }
            else if (it.media_type === "image") {
                mediaTiles.push({ type: "image", key: it.media_key || "", width: it.width, height: it.height })
            } else if (it.media_type === "video") {
                mediaTiles.push({ type: "video", key: it.media_key || "", duration: it.duration_ms || 0 })
            }
        }
        momentText = textParts.join("\n")
        momentMediaTiles = mediaTiles

        var rawLikes = snap.likes || [], names = []
        for (var j = 0; j < rawLikes.length; j++) {
            if (rawLikes[j].user_nick) names.push(rawLikes[j].user_nick)
        }
        likeNames = names
        commentList = snap.comments || []
    }

    function openMoment(momentId) {
        console.log("[MomentsDetail] openMoment called, momentId=" + momentId + " controller=" + !!root.controller)
        root.currentMomentId = momentId
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
                    anchors.fill: parent; fillMode: Image.Contain
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
