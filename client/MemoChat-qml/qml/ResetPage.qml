import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"

Rectangle {
    id: resetRoot
    radius: 0
    antialiasing: false
    clip: false
    color: "transparent"

    GlassBackdrop {
        id: backdropLayer
        anchors.fill: parent
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 18
        anchors.bottomMargin: 18
        spacing: 10

        Item { Layout.fillWidth: true; Layout.preferredHeight: 12 }

        Label {
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: controller.tipText
            color: controller.tipError ? "#d64545" : "#2a7f62"
            font.pixelSize: 13
            visible: text.length > 0
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "用户名："
                color: "#5e6472"
                Layout.preferredWidth: 60
                horizontalAlignment: Text.AlignRight
            }

            GlassTextField {
                id: userField
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                backdrop: backdropLayer
                placeholderText: "输入用户名"
                onTextChanged: controller.clearTip()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "邮箱："
                color: "#5e6472"
                Layout.preferredWidth: 60
                horizontalAlignment: Text.AlignRight
            }

            GlassTextField {
                id: emailField
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                backdrop: backdropLayer
                placeholderText: "输入邮箱"
                onTextChanged: controller.clearTip()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "验证码："
                color: "#5e6472"
                Layout.preferredWidth: 60
                horizontalAlignment: Text.AlignRight
            }

            GlassTextField {
                id: verifyField
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                backdrop: backdropLayer
                placeholderText: "输入验证码"
                onTextChanged: controller.clearTip()
            }

            GlassButton {
                id: resetCodeBtn
                Layout.preferredWidth: 58
                Layout.preferredHeight: 38
                enabled: !controller.busy
                text: "获取"
                textPixelSize: 14
                onClicked: controller.requestResetCode(emailField.text)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "新密码："
                color: "#5e6472"
                Layout.preferredWidth: 60
                horizontalAlignment: Text.AlignRight
            }

            GlassTextField {
                id: pwdField
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                backdrop: backdropLayer
                placeholderText: "输入新密码"
                echoMode: TextInput.Password
                onTextChanged: controller.clearTip()
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            GlassButton {
                id: resetSubmitBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                enabled: !controller.busy
                text: controller.busy ? "处理中..." : "确认"
                onClicked: controller.resetPassword(
                               userField.text,
                               emailField.text,
                               pwdField.text,
                               verifyField.text)
            }

            GlassButton {
                id: resetBackBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                enabled: !controller.busy
                text: "返回"
                onClicked: controller.switchToLogin()
            }
        }

        Item { Layout.fillHeight: true }
    }
}
