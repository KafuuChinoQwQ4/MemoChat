import QtQuick 2.15

Item {
    id: root

    property bool alignRight: false
    property int avatarSize: 34
    property string avatarSource: "qrc:/res/head_1.jpg"
    property string defaultAvatarSource: "qrc:/res/head_1.jpg"
    property int senderUid: 0
    property string senderName: ""
    signal avatarClicked(int uid, string name, string icon)

    Rectangle {
        width: root.avatarSize
        height: root.avatarSize
        radius: width / 2
        clip: true
        anchors.left: root.alignRight ? undefined : parent.left
        anchors.right: root.alignRight ? parent.right : undefined
        anchors.bottom: parent.bottom
        color: Qt.rgba(0.73, 0.82, 0.92, 0.50)
        border.color: Qt.rgba(1, 1, 1, 0.56)

        Image {
            id: fallbackImage
            anchors.fill: parent
            anchors.margins: Math.max(4, Math.round(root.avatarSize * 0.18))
            source: "qrc:/icons/user.png"
            fillMode: Image.PreserveAspectFit
            cache: true
            mipmap: true
            opacity: avatarImage.status === Image.Ready ? 0.0 : 0.78
            Behavior on opacity { NumberAnimation { duration: 160 } }
        }

        Image {
            id: avatarImage
            anchors.fill: parent
            property string baseSource: root.avatarSource.length > 0 ? root.avatarSource : root.defaultAvatarSource
            property bool loadFailed: false
            source: loadFailed ? root.defaultAvatarSource : baseSource
            fillMode: Image.PreserveAspectCrop
            cache: true
            asynchronous: true
            opacity: (status === Image.Ready) ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 200 } }
            onBaseSourceChanged: loadFailed = false
            onStatusChanged: {
                if (status === Image.Error) {
                    loadFailed = true
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.avatarClicked(root.senderUid, root.senderName, root.avatarSource)
        }
    }
}
