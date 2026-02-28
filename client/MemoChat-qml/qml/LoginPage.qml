import QtQuick 2.15
import QtQuick.Controls 2.15
import Qt5Compat.GraphicalEffects

Rectangle {
    radius: 20
    antialiasing: true

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#cfd3e2" }
        GradientStop { position: 1.0; color: "#c4e5f7" }
    }

    Popup {
        id: morePopup
        width: 108
        height: 94
        padding: 8
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        y: {
            var p = moreOptionsText.mapToItem(parent, 0, 0)
            return p.y - height - 10
        }
        x: {
            var p = moreOptionsText.mapToItem(parent, 0, 0)
            return Math.min(parent.width - width - 10, Math.max(10, p.x - width / 2 + moreOptionsText.width / 2))
        }

        background: Rectangle {
            radius: 10
            color: "#f7f7f7"
            border.color: "#d7d7d7"
            border.width: 1
        }

        Column {
            anchors.fill: parent
            spacing: 6

            Button {
                width: parent.width
                height: 34
                text: "注册账号"
                onClicked: {
                    morePopup.close()
                    controller.switchToRegister()
                }
            }

            Button {
                width: parent.width
                height: 34
                text: "忘记密码"
                onClicked: {
                    morePopup.close()
                    controller.switchToReset()
                }
            }
        }
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: parent.bottom
            leftMargin: 30
            rightMargin: 30
            topMargin: 20
            bottomMargin: 28
        }
        spacing: 14

        Item {
            width: parent.width
            height: 24

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: -8
                anchors.verticalCenterOffset: -4
                spacing: 22

                Item {
                    id: settingsButton
                    width: 14
                    height: 14

                    Image {
                        anchors.fill: parent
                        source: "qrc:/icons/settings.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: controller.clearTip()
                    }
                }

                Item {
                    width: 14
                    height: 14

                    Image {
                        anchors.fill: parent
                        source: "qrc:/icons/close.png"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.quit()
                    }
                }
            }
        }

        Item { width: parent.width; height: 12 }

        Item {
            width: 96
            height: 96
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "transparent"
                border.color: "#ffffffcc"
                border.width: 2
                antialiasing: true
            }

            Item {
                id: avatarClip
                anchors.fill: parent
                anchors.margins: 3

                Image {
                    id: avatarImage
                    anchors.fill: parent
                    source: "qrc:/res/head_1.jpg"
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    mipmap: true
                    visible: false
                }

                OpacityMask {
                    anchors.fill: parent
                    source: avatarImage
                    maskSource: Rectangle {
                        width: avatarClip.width
                        height: avatarClip.height
                        radius: width / 2
                        color: "black"
                    }
                }
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: controller.tipText
            visible: text.length > 0
            color: controller.tipError ? "#d64545" : "#2a7f62"
            font.pixelSize: 13
        }

        Rectangle {
            width: parent.width
            height: 46
            radius: 11
            color: "#f5f6f8"
            border.color: "#e1e2e6"
            border.width: 1

            TextField {
                id: emailField
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                placeholderText: "输入邮箱"
                font.pixelSize: 17
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                selectByMouse: true
                background: Item { }
                onTextChanged: controller.clearTip()
            }
        }

        Rectangle {
            width: parent.width
            height: 46
            radius: 11
            color: "#f5f6f8"
            border.color: "#e1e2e6"
            border.width: 1

            TextField {
                id: pwdField
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                placeholderText: "输入密码"
                font.pixelSize: 17
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                echoMode: TextInput.Password
                selectByMouse: true
                background: Item { }
                onTextChanged: controller.clearTip()
                onAccepted: loginBtn.triggerLogin()
            }
        }

        Row {
            width: parent.width
            height: 24
            spacing: 6

            CheckBox {
                id: termsCheck
                checked: false
                anchors.verticalCenter: parent.verticalCenter
                focusPolicy: Qt.NoFocus
                hoverEnabled: false
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 0
                implicitWidth: 18
                implicitHeight: 18
                indicator: Rectangle {
                    implicitWidth: 18
                    implicitHeight: 18
                    radius: 9
                    border.width: 1
                    border.color: "#8ea0b4"
                    color: termsCheck.checked ? "#4f9edd" : "transparent"

                    Canvas {
                        anchors.fill: parent
                        visible: termsCheck.checked
                        antialiasing: true
                        onVisibleChanged: requestPaint()
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            if (!visible) {
                                return
                            }
                            ctx.strokeStyle = "#ffffff"
                            ctx.lineWidth = 2
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"
                            ctx.beginPath()
                            ctx.moveTo(width * 0.28, height * 0.55)
                            ctx.lineTo(width * 0.45, height * 0.72)
                            ctx.lineTo(width * 0.74, height * 0.36)
                            ctx.stroke()
                        }
                    }
                }
                background: Item { }
                contentItem: Item { }
            }

            Text {
                width: parent.width - termsCheck.implicitWidth - parent.spacing
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment: Text.AlignVCenter
                textFormat: Text.RichText
                font.pixelSize: 11
                color: "#6d7280"
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                text: "已阅读并同意<a href=\"https://example.com/agreement\">服务协议</a>和<a href=\"https://example.com/privacy\">Memo隐私保护指引</a>"
                onLinkActivated: function(link) { Qt.openUrlExternally(link) }
            }
        }

        Rectangle {
            id: loginBtn
            readonly property bool loginReady: emailField.text.trim().length > 0
                                              && pwdField.text.length > 0
                                              && termsCheck.checked
                                              && !controller.busy
            property bool hovering: false
            property bool pressed: false
            width: parent.width
            height: 48
            radius: 10
            color: !loginBtn.loginReady ? "#b6bec9"
                                        : loginBtn.pressed ? "#4f90c9"
                                                           : loginBtn.hovering ? "#6aaee0"
                                                                              : "#7ebeea"

            function triggerLogin() {
                if (!loginReady) {
                    return
                }
                controller.login(emailField.text, pwdField.text)
            }

            Text {
                anchors.centerIn: parent
                text: controller.busy ? "处理中..." : "登录"
                color: loginBtn.loginReady ? "#eaf5ff" : "#dfe5ec"
                font.pixelSize: 16
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                enabled: !controller.busy
                cursorShape: loginBtn.loginReady ? Qt.PointingHandCursor : Qt.ArrowCursor
                onEntered: loginBtn.hovering = true
                onExited: {
                    loginBtn.hovering = false
                    loginBtn.pressed = false
                }
                onPressed: {
                    if (loginBtn.loginReady) {
                        loginBtn.pressed = true
                    }
                }
                onReleased: loginBtn.pressed = false
                onClicked: loginBtn.triggerLogin()
            }
        }

        Item { width: parent.width; height: 18 }

        Row {
            spacing: 14
            anchors.horizontalCenter: parent.horizontalCenter

            Label {
                text: "扫码登录"
                color: "#2c73df"
                font.pixelSize: 13

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: controller.clearTip()
                }
            }

            Label {
                text: "|"
                color: "#b4b4b4"
                font.pixelSize: 13
            }

            Label {
                id: moreOptionsText
                text: "更多选项"
                color: "#2c73df"
                font.pixelSize: 13

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (morePopup.visible) {
                            morePopup.close()
                        } else {
                            morePopup.open()
                        }
                    }
                }
            }
        }
    }
}
