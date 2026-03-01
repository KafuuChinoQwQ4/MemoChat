import QtQuick 2.15
import QtQuick.Controls 2.15
import "components"

Rectangle {
    id: loginRoot
    property string tipText: ""
    property bool tipError: false
    property bool busy: false

    signal loginRequested(string email, string password)
    signal switchToRegisterRequested()
    signal switchToResetRequested()
    signal clearTipRequested()

    radius: 0
    antialiasing: false
    clip: false
    color: "transparent"

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
            width: parent.width
            onSettingsClicked: loginRoot.clearTipRequested()
            onCloseClicked: Qt.quit()
        }

        Item { width: parent.width; height: 12 }

        LoginAvatar {
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: loginRoot.tipText
            visible: text.length > 0
            color: loginRoot.tipError ? "#d64545" : "#2a7f62"
            font.pixelSize: 13
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
            onTextChanged: loginRoot.clearTipRequested()
        }

        GlassTextField {
            id: pwdField
            width: parent.width
            height: 46
            backdrop: backdropLayer
            blurRadius: 28
            cornerRadius: 11
            leftInset: 16
            rightInset: 16
            textPixelSize: 17
            placeholderText: "输入密码"
            echoMode: TextInput.Password
            onTextChanged: loginRoot.clearTipRequested()
            onAccepted: loginBtn.triggerLogin()
        }

        LoginAgreementRow {
            id: termsRow
            width: parent.width
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
