import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0
import "../components"

Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min(480, (parent ? parent.width : 700) - 48)
    height: Math.min(560, (parent ? parent.height : 640) - 48)
    anchors.centerIn: parent

    property var backdrop: null
    property var controller: null
    property var currentMomentId: 0

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        cornerRadius: 14
        blurRadius: 22
        fillColor: Qt.rgba(1, 1, 1, 0.96)
        strokeColor: Qt.rgba(0.85, 0.85, 0.90, 0.6)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // Header
        RowLayout {
            Layout.fillWidth: true
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
                onClicked: root.closed()
            }
        }

        // Content area (scrollable)
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: contentColumn.height

            Column {
                id: contentColumn
                width: parent.width
                spacing: 8

                // Moment content
                Column {
                    width: parent.width
                    spacing: 6

                    // User info
                    RowLayout {
                        width: parent.width
                        spacing: 8

                        Rectangle {
                            width: 38
                            height: 38
                            radius: 6
                            clip: true
                            color: Qt.rgba(0.8, 0.85, 0.95, 0.5)
                            Image {
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectCrop
                                source: momentUserIcon
                                cache: true
                                asynchronous: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Label {
                                text: momentUserNick
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                color: "#1a1a1a"
                            }
                            Label {
                                text: momentLocation || ""
                                font.pixelSize: 11
                                color: "#888888"
                                visible: momentLocation !== ""
                            }
                        }
                    }

                    // Content text
                    Label {
                        width: parent.width
                        text: momentText
                        font.pixelSize: 14
                        color: "#1a1a1a"
                        wrapMode: Text.Wrap
                        visible: momentText !== ""
                    }

                    // Image grid
                    GridLayout {
                        width: parent.width
                        columns: 3
                        rowSpacing: 4
                        columnSpacing: 4
                        Repeater {
                            model: momentImages
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: imgHeight(modelData)
                                radius: 6
                                color: Qt.rgba(0.92, 0.94, 0.97, 1.0)
                                clip: true
                                Image {
                                    anchors.fill: parent
                                    fillMode: Image.Cover
                                    source: imgUrl(modelData.key)
                                    cache: true
                                    asynchronous: true
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        imgPopupLoader.active = true
                                        if (imgPopupLoader.item) {
                                            imgPopupLoader.item.open(modelData.key)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Divider
                Rectangle {
                    width: parent.width
                    height: 1
                    color: Qt.rgba(0.85, 0.85, 0.90, 0.4)
                }

                // Likes section
                Column {
                    width: parent.width
                    spacing: 4
                    visible: likeCount > 0

                    RowLayout {
                        width: parent.width
                        spacing: 6
                        Label {
                            text: "♥ " + likeCount + " 个赞"
                            font.pixelSize: 12
                            color: "#e84141"
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Repeater {
                        model: likeNames
                        delegate: Label {
                            width: parent ? parent.width : implicitWidth
                            text: modelData
                            font.pixelSize: 12
                            color: "#555555"
                            wrapMode: Text.NoWrap
                            elide: Text.ElideRight
                        }
                    }
                }

                // Comments section
                Column {
                    width: parent.width
                    spacing: 6
                    visible: commentCount > 0

                    Label {
                        text: "评论 (" + commentCount + ")"
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: "#1a1a1a"
                    }

                    Repeater {
                        model: commentList
                        delegate: RowLayout {
                            width: parent ? parent.width : implicitWidth
                            spacing: 6

                            Label {
                                text: modelData.nick + ": "
                                font.pixelSize: 12
                                color: "#2a7ae2"
                                font.weight: Font.Medium
                            }
                            Label {
                                text: modelData.content
                                font.pixelSize: 12
                                color: "#1a1a1a"
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }

        // Comment input bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: commentInput
                Layout.fillWidth: true
                placeholderText: "写评论..."
                placeholderTextColor: "#aaaaaa"
                font.pixelSize: 13
                color: "#1a1a1a"
                background: Rectangle {
                    color: Qt.rgba(0.95, 0.95, 0.97, 0.8)
                    radius: 8
                }
                onAccepted: sendComment()
            }

            Button {
                id: sendBtn
                Layout.preferredWidth: 56
                Layout.preferredHeight: 32
                enabled: commentInput.text.length > 0
                background: Rectangle {
                    radius: 8
                    color: sendBtn.enabled ? "#2a7ae2" : Qt.rgba(0.85, 0.88, 0.92, 0.8)
                }
                contentItem: Label {
                    anchors.centerIn: parent
                    text: "发送"
                    color: sendBtn.enabled ? "#ffffff" : "#aaaaaa"
                    font.pixelSize: 13
                }
                onClicked: sendComment()
            }
        }
    }

    // State
    property string momentUserNick: ""
    property string momentUserIcon: "qrc:/res/head_1.jpg"
    property string momentLocation: ""
    property string momentText: ""
    property var momentImages: []
    property int likeCount: 0
    property var likeNames: []
    property int commentCount: 0
    property var commentList: []

    // Comment state
    property int replyUid: 0
    property string replyNick: ""

    function openMoment(momentId) {
        root.currentMomentId = momentId
        root.open()
        // Load comments
        if (root.controller) {
            // Trigger detail load - the controller will emit a signal when loaded
            root.controller.refreshMoment(momentId)
        }
    }

    function sendComment() {
        if (!commentInput.text || commentInput.text.length === 0) return
        if (root.controller && root.currentMomentId > 0) {
            root.controller.addComment(root.currentMomentId, commentInput.text, replyUid)
            commentInput.clear()
            replyUid = 0
            replyNick = ""
        }
    }

    function imgHeight(item) {
        var w = item.width || 200
        var h = item.height || 200
        var aspect = h / (w || 1)
        return Math.min(140, Math.max(60, 120 * aspect))
    }

    function imgUrl(key) {
        if (!key) return ""
        return gate_url_prefix + "/media/download?asset=" + key
    }

    // Image popup
    Loader {
        id: imgPopupLoader
        active: false
        sourceComponent: Component {
            Popup {
                id: imgPopup
                modal: true
                focus: true
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                property string currentKey: ""
                function open(key) { currentKey = key; imgPopup.open() }
                width: parent && parent.width ? parent.width - 40 : 680
                height: parent && parent.height ? parent.height - 40 : 500
                anchors.centerIn: Overlay.overlay
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
