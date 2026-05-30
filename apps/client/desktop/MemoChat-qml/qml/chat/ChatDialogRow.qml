import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root

    property int rowIndex: -1
    property int uidValue: 0
    property string title: ""
    property string iconSource: ""
    property string lastMessage: ""
    property string description: ""
    property string draftText: ""
    property int pinnedRank: 0
    property int muteState: 0
    property int unreadCount: 0
    property bool current: false
    signal activated(int uid, int rowIndex)
    signal pinToggled(int uid)
    signal muteToggled(int uid)
    signal markRead(int uid)
    signal draftCleared(int uid)

    width: ListView.view ? ListView.view.width : 250
    height: 64
    color: root.current ? Qt.rgba(0.77, 0.86, 0.97, 0.32) : "transparent"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 42
            Layout.preferredHeight: 42
            radius: 4
            clip: true
            color: Qt.rgba(0.73, 0.82, 0.92, 0.46)

            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                property bool isGroupDialog: root.uidValue < 0
                property string fallbackSource: isGroupDialog ? "qrc:/res/chat_icon.png" : "qrc:/res/head_1.jpg"
                property string baseSource: (root.iconSource && root.iconSource.length > 0) ? root.iconSource : fallbackSource
                property bool loadFailed: false
                source: loadFailed ? fallbackSource : baseSource
                cache: true
                asynchronous: true
                opacity: status === Image.Ready ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: 200 } }
                onBaseSourceChanged: loadFailed = false
                onStatusChanged: if (status === Image.Error) { loadFailed = true }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3

            Label {
                text: root.title
                font.bold: true
                color: "#273449"
            }

            Label {
                text: root.draftText.length > 0
                      ? ("草稿: " + root.draftText)
                      : (root.lastMessage.length > 0 ? root.lastMessage : root.description)
                color: root.draftText.length > 0 ? "#c05f45" : "#647489"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(mouse) {
            root.activated(root.uidValue, root.rowIndex)
            if (mouse.button === Qt.RightButton) {
                sessionMenu.popup()
            }
        }
    }

    Menu {
        id: sessionMenu
        y: parent.height - 4

        MenuItem {
            text: root.pinnedRank > 0 ? "取消置顶" : "置顶会话"
            onTriggered: root.pinToggled(root.uidValue)
        }

        MenuItem {
            text: root.muteState > 0 ? "取消静音" : "静音会话"
            onTriggered: root.muteToggled(root.uidValue)
        }

        MenuItem {
            text: "标记已读"
            enabled: root.unreadCount > 0
            onTriggered: root.markRead(root.uidValue)
        }

        MenuItem {
            text: "清空草稿"
            enabled: root.draftText.length > 0
            onTriggered: root.draftCleared(root.uidValue)
        }
    }
}
