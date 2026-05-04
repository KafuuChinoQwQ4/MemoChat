import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

GlassSurface {
    id: root
    property string groupName: ""
    property string groupCode: ""
    property string groupIcon: "qrc:/res/chat_icon.png"
    property bool canUpdateIcon: false
    property bool canUpdateAnnouncement: false
    property int currentGroupRole: 1
    property bool currentDialogPinned: false
    property bool currentDialogMuted: false
    property string statusText: ""
    property bool statusError: false

    signal refreshRequested()
    signal loadHistoryRequested()
    signal updateAnnouncementRequested(string announcement)
    signal updateGroupIconRequested(int source)
    signal toggleDialogPinned()
    signal toggleDialogMuted()
    signal quitRequested()
    signal dissolveRequested()

    cornerRadius: 10
    blurRadius: 26
    implicitHeight: infoColumn.implicitHeight + 20
    fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.90)
    strokeColor: Qt.rgba(1, 1, 1, 0.58)
    glowTopColor: Qt.rgba(1, 1, 1, 0.24)
    glowBottomColor: Qt.rgba(1, 1, 1, 0.06)

    ColumnLayout {
        id: infoColumn
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: root.groupName.length > 0 ? root.groupName : "群聊"
            color: "#2a3649"
            font.bold: true
            font.pixelSize: 15
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: root.groupCode.length > 0 ? ("群ID: " + root.groupCode) : "群ID: -"
            color: "#5f6f85"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: 22
                clip: true
                color: Qt.rgba(0.73, 0.82, 0.92, 0.50)
                border.color: Qt.rgba(1, 1, 1, 0.56)

                Image {
                    anchors.fill: parent
                    property string fallbackSource: "qrc:/res/chat_icon.png"
                    property string baseSource: (root.groupIcon && root.groupIcon.length > 0)
                                                ? root.groupIcon
                                                : fallbackSource
                    property bool loadFailed: false
                    source: loadFailed ? fallbackSource : baseSource
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
            }

            GlassButton {
                id: iconBtn
                Layout.preferredWidth: 96
                text: "修改群头像"
                implicitHeight: 30
                enabled: root.canUpdateIcon
                cornerRadius: 8
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: groupIconMenu.open()
            }
        }

        Menu {
            id: groupIconMenu
            width: 160
            MenuItem {
                text: "从相册选择"
                onClicked: root.updateGroupIconRequested(0)
            }
            MenuItem {
                text: "屏幕截图"
                onClicked: root.updateGroupIconRequested(1)
            }
            MenuItem {
                text: "拍照上传"
                onClicked: root.updateGroupIconRequested(2)
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.statusText
            visible: text.length > 0
            wrapMode: Text.Wrap
            color: root.statusError ? "#cc4a4a" : "#2a7f62"
            font.pixelSize: 12
        }

        Label {
            Layout.fillWidth: true
            text: root.currentGroupRole >= 3 ? "群主可解散群聊" : "仅群主可解散群聊"
            color: "#60738a"
            font.pixelSize: 11
            wrapMode: Text.Wrap
        }

        GlassTextField {
            id: announcementInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "群公告（最多1000字）"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            GlassButton {
                Layout.fillWidth: true
                text: "刷新"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.refreshRequested()
            }
            GlassButton {
                Layout.fillWidth: true
                text: "历史"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.loadHistoryRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            GlassButton {
                Layout.fillWidth: true
                text: root.currentDialogPinned ? "取消置顶" : "置顶会话"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.toggleDialogPinned()
            }

            GlassButton {
                Layout.fillWidth: true
                text: root.currentDialogMuted ? "取消静音" : "静音会话"
                implicitHeight: 30
                cornerRadius: 8
                normalColor: Qt.rgba(0.42, 0.56, 0.74, 0.22)
                hoverColor: Qt.rgba(0.42, 0.56, 0.74, 0.32)
                pressedColor: Qt.rgba(0.42, 0.56, 0.74, 0.40)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.toggleDialogMuted()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            GlassButton {
                Layout.fillWidth: true
                text: "更新公告"
                implicitHeight: 30
                enabled: root.canUpdateAnnouncement
                cornerRadius: 8
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.updateAnnouncementRequested(announcementInput.text)
            }
            GlassButton {
                Layout.fillWidth: true
                text: "退群"
                implicitHeight: 32
                enabled: root.currentGroupRole < 3
                cornerRadius: 8
                normalColor: Qt.rgba(0.82, 0.38, 0.38, 0.22)
                hoverColor: Qt.rgba(0.82, 0.38, 0.38, 0.32)
                pressedColor: Qt.rgba(0.82, 0.38, 0.38, 0.40)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.quitRequested()
            }
        }

        GlassButton {
            Layout.fillWidth: true
            text: "解散群聊"
            implicitHeight: 34
            visible: root.currentGroupRole >= 3
            enabled: root.currentGroupRole >= 3
            cornerRadius: 9
            normalColor: Qt.rgba(0.82, 0.28, 0.34, 0.22)
            hoverColor: Qt.rgba(0.82, 0.28, 0.34, 0.32)
            pressedColor: Qt.rgba(0.82, 0.28, 0.34, 0.42)
            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
            onClicked: root.dissolveRequested()
        }
    }
}
