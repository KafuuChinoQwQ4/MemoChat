import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    radius: 20
    antialiasing: true
    color: "#f3f4f6"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Item { Layout.fillWidth: true; Layout.preferredHeight: 32 }

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
            Label { text: "用户名："; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
            TextField { id: userField; Layout.fillWidth: true; Layout.preferredHeight: 28; onTextChanged: controller.clearTip() }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { text: "邮箱："; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
            TextField { id: emailField; Layout.fillWidth: true; Layout.preferredHeight: 28; onTextChanged: controller.clearTip() }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { text: "验证码："; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
            TextField { id: verifyField; Layout.fillWidth: true; Layout.preferredHeight: 28; onTextChanged: controller.clearTip() }
            Button {
                text: "获取"
                Layout.preferredHeight: 28
                enabled: !controller.busy
                onClicked: controller.requestResetCode(emailField.text)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label { text: "新密码："; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight }
            TextField {
                id: pwdField
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                echoMode: TextInput.Password
                onTextChanged: controller.clearTip()
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                text: controller.busy ? "处理中..." : "确认"
                enabled: !controller.busy
                onClicked: controller.resetPassword(
                               userField.text,
                               emailField.text,
                               pwdField.text,
                               verifyField.text)
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                text: "返回"
                enabled: !controller.busy
                onClicked: controller.switchToLogin()
            }
        }

        Item { Layout.fillHeight: true }
    }
}
