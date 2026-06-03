import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: root
    spacing: 0

    property int likeCount: 0
    property int commentCount: 0
    property bool hasLiked: false

    signal likeClicked()
    signal commentClicked()

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
