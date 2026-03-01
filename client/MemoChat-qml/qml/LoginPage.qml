import QtQuick 2.15
import QtQuick.Controls 2.15
import "components"

Rectangle {
    id: loginRoot
    property string tipText: ""
    property bool tipError: false
    property bool busy: false
    property bool pwdVisible: false

    signal loginRequested(string email, string password)
    signal switchToRegisterRequested()
    signal switchToResetRequested()
    signal clearTipRequested()
    property real revealProgress: 0.0

    radius: 0
    antialiasing: false
    clip: false
    color: "transparent"

    RegularExpressionValidator {
        id: emailInputValidator
        regularExpression: /^[A-Za-z0-9@._%+\-]*$/
    }

    RegularExpressionValidator {
        id: passwordInputValidator
        regularExpression: /^[A-Za-z0-9!@#$%^&*._+\-=~?]{0,15}$/
    }

    function stageValue(start, span) {
        return Math.max(0, Math.min(1, (revealProgress - start) / span))
    }

    Component.onCompleted: revealProgress = 1.0

    Behavior on revealProgress {
        NumberAnimation {
            duration: 760
            easing.type: Easing.OutCubic
        }
    }

    GlassBackdrop {
        id: backdropLayer
        anchors.fill: parent
    }

    LoginMoreOptionsPopup {
        id: morePopup
        hostItem: loginRoot
        anchorItem: footerLinks.moreOptionsAnchor
        backdrop: backdropLayer
        onRegisterClicked: loginRoot.switchToRegisterRequested()
        onResetClicked: loginRoot.switchToResetRequested()
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

        LoginTopBar {
            id: topBar
            width: parent.width
            opacity: loginRoot.stageValue(0.02, 0.16)
            scale: 0.97 + 0.03 * opacity
            onSettingsClicked: loginRoot.clearTipRequested()
            onCloseClicked: Qt.quit()
        }

        Item { width: parent.width; height: 12 }

        LoginAvatar {
            id: avatarSection
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: loginRoot.stageValue(0.09, 0.16)
            scale: 0.96 + 0.04 * opacity
        }

        Label {
            id: tipLabel
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: loginRoot.tipText
            visible: text.length > 0
            color: loginRoot.tipError ? "#d64545" : "#2a7f62"
            font.pixelSize: 13
            opacity: loginRoot.stageValue(0.20, 0.16)

            Behavior on color {
                ColorAnimation {
                    duration: 150
                    easing.type: Easing.OutQuad
                }
            }
        }

        GlassTextField {
            id: emailField
            width: parent.width
            height: 46
            backdrop: backdropLayer
            blurRadius: 28
            cornerRadius: 11
            leftInset: 16
            rightInset: 16
            textPixelSize: 17
            placeholderText: "输入邮箱"
            inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
            maximumLength: 128
            validator: emailInputValidator
            opacity: loginRoot.stageValue(0.19, 0.18)
            scale: 0.97 + 0.03 * opacity
            onTextChanged: loginRoot.clearTipRequested()
        }

        Item {
            width: parent.width
            height: 46
            opacity: loginRoot.stageValue(0.28, 0.18)
            scale: 0.97 + 0.03 * opacity

            GlassTextField {
                id: pwdField
                anchors.fill: parent
                backdrop: backdropLayer
                blurRadius: 28
                cornerRadius: 11
                leftInset: 16
                rightInset: 44
                textPixelSize: 17
                placeholderText: "输入密码"
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                maximumLength: 15
                validator: passwordInputValidator
                echoMode: loginRoot.pwdVisible ? TextInput.Normal : TextInput.Password
                onTextChanged: loginRoot.clearTipRequested()
                onAccepted: loginBtn.triggerLogin()
            }

            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 12
                width: 20
                height: 20
                source: loginRoot.pwdVisible ? "qrc:/res/visible.png" : "qrc:/res/unvisible.png"
            }

            MouseArea {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 10
                width: 24
                height: 24
                cursorShape: Qt.PointingHandCursor
                onClicked: loginRoot.pwdVisible = !loginRoot.pwdVisible
            }
        }

        LoginAgreementRow {
            id: termsRow
            width: parent.width
            opacity: loginRoot.stageValue(0.36, 0.18)
            scale: 0.98 + 0.02 * opacity
            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
        }

        LoginSubmitButton {
            id: loginBtn
            width: parent.width
            backdrop: backdropLayer
            ready: emailField.text.trim().length > 0
                   && pwdField.text.length > 0
                   && termsRow.checked
                   && !loginRoot.busy
            busy: loginRoot.busy
            opacity: loginRoot.stageValue(0.44, 0.18)
            scale: 0.97 + 0.03 * opacity

            function triggerLogin() {
                if (!ready) {
                    return
                }
                loginRoot.loginRequested(emailField.text, pwdField.text)
            }

            onTriggered: triggerLogin()
        }

        Item { width: parent.width; height: 18 }

        LoginFooterLinks {
            id: footerLinks
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: loginRoot.stageValue(0.54, 0.18)
            scale: 0.98 + 0.02 * opacity
            onScanLoginClicked: loginRoot.clearTipRequested()
            onMoreOptionsClicked: {
                if (morePopup.visible) {
                    morePopup.close()
                } else {
                    Qt.callLater(function() {
                        morePopup.reposition()
                        morePopup.open()
                    })
                }
            }
        }
    }
}
