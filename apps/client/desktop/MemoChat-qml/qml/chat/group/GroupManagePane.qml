import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

GlassSurface {
    id: root

    property bool canInviteUsers: false
    property bool canManageAdmins: false
    property bool canBanUsers: false
    property var friendModel: null
    property string selectedFriendUserId: ""

    signal inviteRequested(string userId, string reason)
    signal setAdminRequested(string userId, bool isAdmin, int permissionBits)
    signal muteRequested(string userId, int muteSeconds)
    signal kickRequested(string userId)

    function syncTargetFromFriend(indexValue) {
        if (!friendModel || indexValue < 0 || !friendModel.get) {
            return
        }
        const item = friendModel.get(indexValue)
        if (!item || !item.userId || item.userId.length === 0) {
            return
        }
        selectedFriendUserId = item.userId
        uidInput.text = item.userId
    }

    function composePermissionBits() {
        let bits = 0
        if (permChangeInfo.checked) bits |= 1
        if (permDeleteMsg.checked) bits |= 2
        if (permInvite.checked) bits |= 4
        if (permManageAdmins.checked) bits |= 8
        if (permPinMsg.checked) bits |= 16
        if (permBanUsers.checked) bits |= 32
        if (permManageTopics.checked) bits |= 64
        return bits
    }

    component PermissionCheckBox: CheckBox {
        id: control
        hoverEnabled: true
        leftPadding: 0
        rightPadding: 0
        spacing: 6

        indicator: Rectangle {
            implicitWidth: 16
            implicitHeight: 16
            radius: 8
            border.width: 1
            border.color: control.down ? Qt.rgba(0.44, 0.59, 0.80, 0.86)
                                       : control.hovered ? Qt.rgba(0.51, 0.66, 0.86, 0.82)
                                                         : Qt.rgba(0.56, 0.66, 0.78, 0.72)
            color: {
                if (!control.enabled) {
                    return control.checked ? Qt.rgba(0.45, 0.58, 0.74, 0.30) : Qt.rgba(1, 1, 1, 0.08)
                }
                if (control.checked) {
                    if (control.down) {
                        return Qt.rgba(0.31, 0.62, 0.90, 0.72)
                    }
                    if (control.hovered) {
                        return Qt.rgba(0.35, 0.67, 0.94, 0.66)
                    }
                    return Qt.rgba(0.33, 0.64, 0.92, 0.60)
                }
                if (control.down) {
                    return Qt.rgba(0.33, 0.64, 0.92, 0.30)
                }
                if (control.hovered) {
                    return Qt.rgba(0.33, 0.64, 0.92, 0.16)
                }
                return Qt.rgba(1, 1, 1, 0.08)
            }
            scale: control.down ? 0.96 : (control.hovered ? 1.02 : 1.0)

            Behavior on color {
                ColorAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }
            Behavior on scale {
                NumberAnimation {
                    duration: 110
                    easing.type: Easing.OutQuad
                }
            }

            Text {
                anchors.centerIn: parent
                text: control.checked ? "✓" : ""
                color: "#ffffff"
                font.pixelSize: 11
                font.bold: true
            }
        }

        contentItem: Text {
            text: control.text
            color: control.enabled ? "#32465f" : "#8b96a5"
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
        }

        HoverHandler {
            cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }

    cornerRadius: 10
    blurRadius: 26
    implicitHeight: manageColumn.implicitHeight + 20
    fillColor: Qt.rgba(1, 1, 1, 0.20)
    strokeColor: Qt.rgba(1, 1, 1, 0.42)

    ColumnLayout {
        id: manageColumn
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: "群管理"
            color: "#2a3649"
            font.bold: true
            font.pixelSize: 14
        }

        ComboBox {
            id: friendCombo
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            model: root.friendModel
            textRole: "nick"
            valueRole: "userId"
            enabled: root.friendModel && root.friendModel.count > 0
            currentIndex: -1
            displayText: currentIndex >= 0 ? currentText : "选择好友"
            onActivated: root.syncTargetFromFriend(index)

            background: Rectangle {
                radius: 9
                color: friendCombo.enabled ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(0.52, 0.57, 0.64, 0.12)
                border.color: friendCombo.hovered ? Qt.rgba(0.35, 0.61, 0.90, 0.48) : Qt.rgba(1, 1, 1, 0.34)
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: friendCombo.displayText
                color: friendCombo.enabled ? "#2a3649" : "#8b96a5"
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        GlassTextField {
            id: uidInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "目标用户ID（可手动补充）"
        }

        GlassTextField {
            id: reasonInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "理由（可选）"
        }

        GlassTextField {
            id: muteInput
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            backdrop: root.backdrop
            placeholderText: "禁言秒数（0=解禁）"
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 2
            columnSpacing: 8

            PermissionCheckBox { id: permChangeInfo; text: "改群资料"; checked: true; enabled: root.canManageAdmins }
            PermissionCheckBox { id: permDeleteMsg; text: "删消息"; checked: true; enabled: root.canManageAdmins }
            PermissionCheckBox { id: permInvite; text: "邀成员"; checked: true; enabled: root.canManageAdmins }
            PermissionCheckBox { id: permManageAdmins; text: "管管理员"; checked: false; enabled: root.canManageAdmins }
            PermissionCheckBox { id: permPinMsg; text: "置顶消息"; checked: true; enabled: root.canManageAdmins }
            PermissionCheckBox { id: permBanUsers; text: "禁言踢人"; checked: true; enabled: root.canManageAdmins }
            PermissionCheckBox { id: permManageTopics; text: "管话题(预留)"; checked: false; enabled: root.canManageAdmins }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            GlassButton {
                Layout.fillWidth: true
                text: "邀请"
                implicitHeight: 30
                enabled: root.canInviteUsers
                cornerRadius: 8
                normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24)
                hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34)
                pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.inviteRequested(uidInput.text.trim(), reasonInput.text)
            }
            GlassButton {
                Layout.fillWidth: true
                text: "设管理员"
                implicitHeight: 30
                enabled: root.canManageAdmins
                cornerRadius: 8
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.setAdminRequested(uidInput.text.trim(), true, root.composePermissionBits())
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            GlassButton {
                Layout.fillWidth: true
                text: "撤管理员"
                implicitHeight: 30
                enabled: root.canManageAdmins
                cornerRadius: 8
                normalColor: Qt.rgba(0.48, 0.56, 0.66, 0.20)
                hoverColor: Qt.rgba(0.48, 0.56, 0.66, 0.30)
                pressedColor: Qt.rgba(0.48, 0.56, 0.66, 0.38)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.setAdminRequested(uidInput.text.trim(), false, 0)
            }
            GlassButton {
                Layout.fillWidth: true
                text: "禁言"
                implicitHeight: 30
                enabled: root.canBanUsers
                cornerRadius: 8
                normalColor: Qt.rgba(0.83, 0.61, 0.24, 0.24)
                hoverColor: Qt.rgba(0.83, 0.61, 0.24, 0.34)
                pressedColor: Qt.rgba(0.83, 0.61, 0.24, 0.42)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.muteRequested(uidInput.text.trim(), parseInt(muteInput.text))
            }
        }

        GlassButton {
            Layout.fillWidth: true
            text: "踢出群聊"
            implicitHeight: 30
            enabled: root.canBanUsers
            cornerRadius: 8
            normalColor: Qt.rgba(0.82, 0.38, 0.38, 0.22)
            hoverColor: Qt.rgba(0.82, 0.38, 0.38, 0.32)
            pressedColor: Qt.rgba(0.82, 0.38, 0.38, 0.40)
            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
            onClicked: root.kickRequested(uidInput.text.trim())
        }
    }
}
