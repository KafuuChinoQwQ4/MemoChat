pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property var commentModel: null
    property bool commentsLoading: false
    property int commentCount: 0
    property int currentUserUid: 0

    signal replyRequested(int uid, string nick)
    signal likeToggled(int commentId, bool liked)
    signal deleteRequested(int commentId)

    height: Math.max(64, commentColumn.implicitHeight + 20)
    color: "#ffffff"
    border.color: Qt.rgba(0.82, 0.84, 0.90, 0.5)
    border.width: 1
    radius: 10

    readonly property int modelCount: commentModel ? commentModel.count : 0

    Column {
        id: commentColumn
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6

        Row {
            spacing: 4

            Label {
                text: "评论"
                font.pixelSize: 13
                font.weight: Font.Medium
                color: "#1a1a1a"
            }

            Label {
                text: "(" + root.commentCount + ")"
                font.pixelSize: 13
                color: "#888888"
                visible: root.commentCount > 0
            }
        }

        Label {
            text: "暂无评论，快来抢沙发"
            font.pixelSize: 12
            color: "#bbbbbb"
            visible: !root.commentsLoading && root.modelCount === 0
        }

        Label {
            text: "评论加载中..."
            font.pixelSize: 12
            color: "#999999"
            visible: root.commentsLoading && root.modelCount === 0
        }

        Column {
            width: parent.width
            spacing: 8
            visible: root.modelCount > 0

            Repeater {
                model: root.commentModel
                delegate: commentItem
            }
        }
    }

    Component {
        id: commentItem

        Rectangle {
            id: commentRoot

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

            width: parent ? parent.width : 0
            height: commentCol.implicitHeight + 16
            color: Qt.rgba(0.96, 0.98, 1.0, 0.92)
            radius: 10
            border.color: Qt.rgba(0.82, 0.86, 0.92, 0.45)

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
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: "#2a7ae2"
                        Layout.maximumWidth: parent.width * 0.42
                        elide: Text.ElideRight
                    }

                    Label {
                        text: commentRoot.targetReplyUid > 0 ? (" 回复 " + commentRoot.targetReplyNick) : ""
                        font.pixelSize: 12
                        color: "#2a7ae2"
                        visible: commentRoot.targetReplyUid > 0
                        Layout.maximumWidth: parent.width * 0.32
                        elide: Text.ElideRight
                    }

                    Label {
                        text: "："
                        font.pixelSize: 12
                        color: "#555555"
                    }

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
                    font.pixelSize: 13
                    color: "#1a1a1a"
                    wrapMode: Text.Wrap
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
                    onTriggered: root.replyRequested(commentRoot.commentUid, commentRoot.commentNick)
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
                    onTriggered: root.likeToggled(commentRoot.commentId, !commentRoot.likedByMe)
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
                    onTriggered: root.deleteRequested(commentRoot.commentId)
                }
            }
        }
    }
}
