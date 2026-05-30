pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property int replyUid: 0
    property string replyNick: ""
    property bool commentSending: false
    property string placeholderText: "写评论..."

    signal replyCleared()
    signal commentSubmitted(string text)

    height: replyUid > 0 ? 92 : 64
    color: "#ffffff"
    clip: true

    function clearInput() {
        commentInput.clear()
    }

    function focusInput() {
        commentInput.forceActiveFocus()
    }

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
            visible: root.replyUid > 0
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
                    text: "回复 " + (root.replyNick || "用户")
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
                    onClicked: root.replyCleared()
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
                placeholderText: root.placeholderText
                placeholderTextColor: "#aaaaaa"
                font.pixelSize: 14
                color: "#1a1a1a"
                background: Rectangle {
                    color: "#f5f6f9"
                    radius: 12
                    border.color: Qt.rgba(0.82, 0.84, 0.90, 0.4)
                    border.width: 1
                }
                leftPadding: 12
                rightPadding: 12
                verticalAlignment: TextInput.AlignVCenter
                onAccepted: root.commentSubmitted(text.trim())
            }

            Button {
                id: sendBtn
                Layout.preferredWidth: 64
                Layout.fillHeight: true
                enabled: commentInput.text.trim().length > 0 && !root.commentSending
                background: Rectangle {
                    radius: 12
                    color: sendBtn.enabled ? "#2a7ae2" : Qt.rgba(0.82, 0.85, 0.90, 1.0)
                }
                contentItem: Label {
                    anchors.centerIn: parent
                    text: root.commentSending ? "..." : "发送"
                    color: sendBtn.enabled ? "#ffffff" : "#aaaaaa"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.commentSubmitted(commentInput.text.trim())
            }
        }
    }
}
