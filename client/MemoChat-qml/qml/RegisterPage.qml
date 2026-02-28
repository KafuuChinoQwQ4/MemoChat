import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    radius: 20
    antialiasing: true
    color: "#f3f4f6"

    StackLayout {
        anchors.fill: parent
        anchors.margins: 10
        currentIndex: controller.registerSuccessPage ? 1 : 0

        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 8

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

                Item { Layout.fillWidth: true; Layout.preferredHeight: 8 }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "用户："; Layout.preferredWidth: 48; horizontalAlignment: Text.AlignRight }
                    TextField { id: userField; Layout.fillWidth: true; Layout.preferredHeight: 28; onTextChanged: controller.clearTip() }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "邮箱："; Layout.preferredWidth: 48; horizontalAlignment: Text.AlignRight }
                    TextField { id: emailField; Layout.fillWidth: true; Layout.preferredHeight: 28; onTextChanged: controller.clearTip() }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "密码："; Layout.preferredWidth: 48; horizontalAlignment: Text.AlignRight }
                    TextField {
                        id: pwdField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        echoMode: pwdVisible.checked ? TextInput.Normal : TextInput.Password
                        onTextChanged: controller.clearTip()
                    }
                    CheckBox {
                        id: pwdVisible
                        indicator: Image {
                            source: pwdVisible.checked ? "qrc:/res/visible.png" : "qrc:/res/unvisible.png"
                            width: 20
                            height: 20
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "确认："; Layout.preferredWidth: 48; horizontalAlignment: Text.AlignRight }
                    TextField {
                        id: confirmField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        echoMode: confirmVisible.checked ? TextInput.Normal : TextInput.Password
                        onTextChanged: controller.clearTip()
                    }
                    CheckBox {
                        id: confirmVisible
                        indicator: Image {
                            source: confirmVisible.checked ? "qrc:/res/visible.png" : "qrc:/res/unvisible.png"
                            width: 20
                            height: 20
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Label { text: "验证码："; Layout.preferredWidth: 48; horizontalAlignment: Text.AlignRight }
                    TextField { id: verifyField; Layout.fillWidth: true; Layout.preferredHeight: 28; onTextChanged: controller.clearTip() }
                    Button {
                        text: "获取"
                        Layout.preferredHeight: 28
                        enabled: !controller.busy
                        onClicked: controller.requestRegisterCode(emailField.text)
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
                        onClicked: controller.registerUser(
                                       userField.text,
                                       emailField.text,
                                       pwdField.text,
                                       confirmField.text,
                                       verifyField.text)
                    }
                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        text: "取消"
                        enabled: !controller.busy
                        onClicked: controller.switchToLogin()
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 16

                Item { Layout.fillHeight: true }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: "注册成功，" + controller.registerCountdown + " s后返回登录"
                }
                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: "可点击返回按钮直接返回登录界面"
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 30
                    text: "返回登录"
                    onClicked: controller.switchToLogin()
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
