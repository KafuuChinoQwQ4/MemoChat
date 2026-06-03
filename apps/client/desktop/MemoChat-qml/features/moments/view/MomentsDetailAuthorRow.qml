import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: root
    spacing: 10

    property string userNick: ""
    property string userName: ""
    property string userIcon: "qrc:/res/head_1.jpg"
    property string locationText: ""
    property int uid: 0
    property string userId: ""

    signal avatarProfileRequested(int uid, string name, string icon, string userId)

    Rectangle {
        Layout.preferredWidth: 42
        Layout.preferredHeight: 42
        radius: 10
        clip: true
        color: Qt.rgba(0.85, 0.88, 0.97, 1.0)

        Image {
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            source: root.userIcon
            cache: true
            asynchronous: true
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.avatarProfileRequested(root.uid,
                                                   root.userNick || root.userName || "用户",
                                                   root.userIcon,
                                                   root.userId)
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 3

        Label {
            Layout.fillWidth: true
            text: root.userNick || " "
            font.pixelSize: 14
            font.weight: Font.Medium
            color: "#1a1a1a"
            wrapMode: Text.Wrap
        }

        Label {
            Layout.fillWidth: true
            text: root.locationText
            font.pixelSize: 12
            color: "#888888"
            visible: text.length > 0
        }
    }
}
