import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Popup {
    id: root
    modal: true
    focus: true
    width: 420
    height: 560
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    property Item backdrop: null
    property var friendModel: null
    property var selectedUserIds: ({})
    readonly property int selectedCount: {
        var count = 0
        for (var key in selectedUserIds) {
            if (selectedUserIds[key]) {
                ++count
            }
        }
        return count
    }
    signal submitted(string name, var memberUserIds)

    function isSelected(userId) {
        return userId && userId.length > 0 && !!selectedUserIds[userId]
    }

    function toggleFriend(userId) {
        if (!userId || userId.length === 0) {
            return
        }
        var next = {}
        for (var key in selectedUserIds) {
            if (selectedUserIds[key]) {
                next[key] = true
            }
        }
        if (next[userId]) {
            delete next[userId]
        } else {
            next[userId] = true
        }
        selectedUserIds = next
    }

    function collectMemberUserIds() {
        var ids = []
        var seen = {}
        for (var key in selectedUserIds) {
            if (selectedUserIds[key] && !seen[key]) {
                ids.push(key)
                seen[key] = true
            }
        }

        var parts = manualMembersInput.text.split(",")
        for (var i = 0; i < parts.length; ++i) {
            var one = parts[i].trim()
            if (one.length > 0 && !seen[one]) {
                ids.push(one)
                seen[one] = true
            }
        }
        return ids
    }

    background: GlassSurface {
        backdrop: root.backdrop
        cornerRadius: 12
        blurRadius: 30
        fillColor: Qt.rgba(1, 1, 1, 0.22)
        strokeColor: Qt.rgba(1, 1, 1, 0.42)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Label {
            text: "创建群聊"
            font.pixelSize: 18
            font.bold: true
            color: "#2a3649"
        }

        GlassTextField {
            id: groupNameInput
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            backdrop: root.backdrop
            placeholderText: "群名称（1-64）"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: "选择好友"
                color: "#56677d"
                font.pixelSize: 12
                font.bold: true
            }

            Label {
                text: root.selectedCount > 0 ? ("已选 " + root.selectedCount) : "可选"
                color: root.selectedCount > 0 ? "#2f7d69" : "#708197"
                font.pixelSize: 12
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 210
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.20)
            border.color: Qt.rgba(1, 1, 1, 0.36)
            clip: true

            ListView {
                id: friendListView
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                spacing: 6
                model: root.friendModel

                delegate: Rectangle {
                    id: friendRow
                    width: ListView.view.width
                    height: 48
                    radius: 9
                    color: rowArea.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.22)
                                           : rowArea.containsMouse ? Qt.rgba(0.35, 0.61, 0.90, 0.14)
                                                                   : root.isSelected(userId) ? Qt.rgba(0.28, 0.70, 0.58, 0.16)
                                                                                             : Qt.rgba(1, 1, 1, 0.16)
                    border.color: root.isSelected(userId) ? Qt.rgba(0.28, 0.70, 0.58, 0.46)
                                                          : Qt.rgba(1, 1, 1, 0.30)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 10
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 34
                            Layout.preferredHeight: 34
                            radius: 17
                            color: Qt.rgba(0.86, 0.91, 0.98, 0.62)
                            border.color: Qt.rgba(1, 1, 1, 0.46)
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: icon && icon.length > 0 ? icon : "qrc:/res/head_1.jpg"
                                fillMode: Image.PreserveAspectCrop
                                sourceSize.width: 68
                                sourceSize.height: 68
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Label {
                                Layout.fillWidth: true
                                text: nick && nick.length > 0 ? nick : (name && name.length > 0 ? name : "好友")
                                color: "#2a3649"
                                font.pixelSize: 13
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: userId && userId.length > 0 ? userId : ("UID: " + uid)
                                color: "#6a7b92"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            radius: 10
                            color: root.isSelected(userId) ? Qt.rgba(0.28, 0.70, 0.58, 0.74)
                                                           : Qt.rgba(1, 1, 1, 0.18)
                            border.color: root.isSelected(userId) ? Qt.rgba(0.28, 0.70, 0.58, 0.88)
                                                                  : Qt.rgba(0.56, 0.66, 0.78, 0.68)

                            Text {
                                anchors.centerIn: parent
                                text: root.isSelected(userId) ? "✓" : ""
                                color: "#ffffff"
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }

                    MouseArea {
                        id: rowArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.toggleFriend(userId)
                    }

                    Behavior on color {
                        ColorAnimation { duration: 110; easing.type: Easing.OutQuad }
                    }
                }

                ScrollBar.vertical: GlassScrollBar { }
            }

            Label {
                anchors.centerIn: parent
                visible: !root.friendModel || root.friendModel.count === 0
                text: "暂无好友，可在下方手动填写用户ID"
                color: "#73859b"
                font.pixelSize: 12
            }
        }

        Label {
            text: "手动补充用户ID（可选，英文逗号分隔）"
            color: "#56677d"
            font.pixelSize: 12
        }

        TextArea {
            id: manualMembersInput
            Layout.fillWidth: true
            Layout.preferredHeight: 62
            placeholderText: "例如：u123456789,u223456789"
            wrapMode: TextArea.Wrap
            color: "#2a3649"
            selectByMouse: true
            font.pixelSize: 13
            background: Rectangle {
                radius: 10
                color: Qt.rgba(1, 1, 1, 0.18)
                border.color: Qt.rgba(1, 1, 1, 0.34)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            GlassButton {
                Layout.fillWidth: true
                text: "取消"
                implicitHeight: 34
                cornerRadius: 8
                normalColor: Qt.rgba(0.48, 0.56, 0.66, 0.20)
                hoverColor: Qt.rgba(0.48, 0.56, 0.66, 0.30)
                pressedColor: Qt.rgba(0.48, 0.56, 0.66, 0.38)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.close()
            }

            GlassButton {
                Layout.fillWidth: true
                text: "创建"
                implicitHeight: 34
                cornerRadius: 8
                normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24)
                hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34)
                pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: {
                    root.submitted(groupNameInput.text, root.collectMemberUserIds())
                    root.close()
                }
            }
        }
    }

    onOpened: {
        groupNameInput.text = ""
        manualMembersInput.text = ""
        selectedUserIds = {}
        groupNameInput.forceActiveFocus()
    }
}
