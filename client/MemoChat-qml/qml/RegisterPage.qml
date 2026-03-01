import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"

Rectangle {
    id: registerRoot
    radius: 0
    antialiasing: false
    clip: false
    color: "transparent"
    property bool pwdVisible: false
    property bool confirmVisible: false

    RegularExpressionValidator {
        id: emailInputValidator
        regularExpression: /^[A-Za-z0-9@._%+\-]*$/
    }

    RegularExpressionValidator {
        id: passwordInputValidator
        regularExpression: /^[A-Za-z0-9!@#$%^&*._+\-=~?]{0,15}$/
    }

    GlassBackdrop {
        id: backdropLayer
        anchors.fill: parent
    }

    StackLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 18
        anchors.bottomMargin: 18
        currentIndex: controller.registerSuccessPage ? 1 : 0

        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 10

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

                Item { Layout.fillWidth: true; Layout.preferredHeight: 4 }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "用户："
                        color: "#5e6472"
                        Layout.preferredWidth: 48
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
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                    }

                    GlassTextField {
                        id: emailField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        backdrop: backdropLayer
                        placeholderText: "输入邮箱"
                        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                        maximumLength: 128
                        validator: emailInputValidator
                        onTextChanged: controller.clearTip()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "密码："
                        color: "#5e6472"
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38

                        GlassTextField {
                            id: pwdField
                            anchors.fill: parent
                            backdrop: backdropLayer
                            placeholderText: "输入密码"
                            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                            maximumLength: 15
                            validator: passwordInputValidator
                            rightInset: 38
                            echoMode: registerRoot.pwdVisible ? TextInput.Normal : TextInput.Password
                            onTextChanged: controller.clearTip()
                        }

                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            width: 20
                            height: 20
                            source: registerRoot.pwdVisible ? "qrc:/res/visible.png" : "qrc:/res/unvisible.png"
                        }

                        MouseArea {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            width: 24
                            height: 24
                            cursorShape: Qt.PointingHandCursor
                            onClicked: registerRoot.pwdVisible = !registerRoot.pwdVisible
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "确认："
                        color: "#5e6472"
                        Layout.preferredWidth: 48
                        horizontalAlignment: Text.AlignRight
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38

                        GlassTextField {
                            id: confirmField
                            anchors.fill: parent
                            backdrop: backdropLayer
                            placeholderText: "确认密码"
                            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                            maximumLength: 15
                            validator: passwordInputValidator
                            rightInset: 38
                            echoMode: registerRoot.confirmVisible ? TextInput.Normal : TextInput.Password
                            onTextChanged: controller.clearTip()
                        }

                        Image {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            width: 20
                            height: 20
                            source: registerRoot.confirmVisible ? "qrc:/res/visible.png" : "qrc:/res/unvisible.png"
                        }

                        MouseArea {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            width: 24
                            height: 24
                            cursorShape: Qt.PointingHandCursor
                            onClicked: registerRoot.confirmVisible = !registerRoot.confirmVisible
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "验证码："
                        color: "#5e6472"
                        Layout.preferredWidth: 48
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
                        id: registerCodeBtn
                        Layout.preferredWidth: 58
                        Layout.preferredHeight: 38
                        enabled: !controller.busy
                        text: "获取"
                        textPixelSize: 14
                        onClicked: controller.requestRegisterCode(emailField.text)
                    }
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    GlassButton {
                        id: registerSubmitBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        enabled: !controller.busy
                        text: controller.busy ? "处理中..." : "确认"
                        onClicked: controller.registerUser(
                                       userField.text,
                                       emailField.text,
                                       pwdField.text,
                                       confirmField.text,
                                       verifyField.text)
                    }

                    GlassButton {
                        id: registerCancelBtn
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        enabled: !controller.busy
                        text: "取消"
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
                    color: "#566074"
                    font.pixelSize: 14
                    text: "注册成功，" + controller.registerCountdown + " s后返回登录"
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    color: "#566074"
                    font.pixelSize: 13
                    text: "可点击返回按钮直接返回登录界面"
                }

                GlassButton {
                    id: backLoginBtn
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 38
                    text: "返回登录"
                    onClicked: controller.switchToLogin()
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
