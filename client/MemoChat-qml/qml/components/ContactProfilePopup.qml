import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: Math.min(360, parent ? parent.width - 48 : 360)
    height: 460
    anchors.centerIn: Overlay.overlay

    property var appController: null
    property int profileUid: 0
    property string profileName: ""
    property string profileNick: ""
    property string profileIcon: "qrc:/res/head_1.jpg"
    property string profileUserId: ""
    property string profileDesc: ""
    property string profileBack: ""
    property int profileSex: 0
    property bool isFriend: false

    function openProfile(uid, fallbackName, fallbackIcon, fallbackUserId) {
        profileUid = uid || 0
        var profile = appController ? appController.contactProfileByUid(profileUid) : ({})
        isFriend = profile && profile.uid !== undefined
        profileName = isFriend ? (profile.name || fallbackName || "用户") : (fallbackName || "用户")
        profileNick = isFriend ? (profile.nick || "") : ""
        profileIcon = isFriend ? (profile.icon || fallbackIcon || "qrc:/res/head_1.jpg") : (fallbackIcon || "qrc:/res/head_1.jpg")
        profileUserId = isFriend ? (profile.userId || fallbackUserId || "") : (fallbackUserId || "")
        profileDesc = isFriend ? (profile.desc || "") : ""
        profileBack = isFriend ? (profile.back || "") : ""
        profileSex = isFriend ? (profile.sex || 0) : 0
        open()
    }

    background: Rectangle {
        color: "#ffffff"
        radius: 16
        border.color: Qt.rgba(0.82, 0.84, 0.90, 0.8)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Label { text: "联系人主页"; font.pixelSize: 17; font.weight: Font.Medium; color: "#1f2937" }
            Item { Layout.fillWidth: true }
            ToolButton { text: "×"; onClicked: root.close() }
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 82; height: 82; radius: 20; clip: true
            color: Qt.rgba(0.85, 0.88, 0.97, 1.0)
            Image { anchors.fill: parent; fillMode: Image.PreserveAspectCrop; source: root.profileIcon }
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: root.profileBack || root.profileNick || root.profileName
            font.pixelSize: 20
            font.weight: Font.Medium
            color: "#111827"
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: root.profileUserId ? ("ID: " + root.profileUserId) : ("UID: " + root.profileUid)
            color: "#6b7280"
            font.pixelSize: 13
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#e5e7eb" }

        Label { Layout.fillWidth: true; text: "昵称：" + (root.profileNick || "未设置"); color: "#374151"; font.pixelSize: 14 }
        Label { Layout.fillWidth: true; text: "备注：" + (root.profileBack || "未设置"); color: "#374151"; font.pixelSize: 14; visible: root.isFriend }
        Label { Layout.fillWidth: true; text: "性别：" + (root.profileSex === 1 ? "女" : (root.profileSex === 2 ? "男" : "未设置")); color: "#374151"; font.pixelSize: 14 }
        Label { Layout.fillWidth: true; text: "签名：" + (root.profileDesc || "暂无"); color: "#374151"; font.pixelSize: 14; wrapMode: Text.Wrap }

        Item { Layout.fillHeight: true }

        Button {
            Layout.fillWidth: true
            visible: root.isFriend
            text: "删除联系人"
            onClicked: confirmDeleteDialog.open()
        }

        Label {
            Layout.fillWidth: true
            visible: !root.isFriend
            text: "该用户还不是你的联系人"
            horizontalAlignment: Text.AlignHCenter
            color: "#9ca3af"
        }
    }

    Dialog {
        id: confirmDeleteDialog
        modal: true
        title: "删除联系人"
        standardButtons: Dialog.Cancel | Dialog.Ok
        anchors.centerIn: Overlay.overlay
        Label {
            text: "确定删除 “" + (root.profileBack || root.profileNick || root.profileName) + "” 吗？"
            wrapMode: Text.Wrap
            width: 260
        }
        onAccepted: {
            if (root.appController) root.appController.deleteFriend(root.profileUid)
            root.close()
        }
    }
}
