import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import "components"

Rectangle {
    id: loginRoot
    property string tipText: ""
    property bool tipError: false
    property bool busy: false
    property bool pwdVisible: false
    property int maxCachedCredentials: 8
    property var credentialProvider: null
    property bool credentialDropdownOpen: false

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

    function parseCredentialCache() {
        try {
            const cacheJson = credentialProvider ? (credentialProvider.loginCredentialCacheJson || "[]") : "[]"
            const parsed = JSON.parse(cacheJson || "[]")
            return Array.isArray(parsed) ? parsed : []
        } catch (e) {
            return []
        }
    }

    function refreshCredentialModel() {
        credentialModel.clear()
        const records = parseCredentialCache()
        for (let i = 0; i < records.length; ++i) {
            const email = String(records[i].email || "").trim()
            const password = String(records[i].password || "")
            if (email.length > 0) {
                credentialModel.append({ "email": email, "password": password })
            }
        }
    }

    function loadLastCredential(fillOnlyEmpty) {
        refreshCredentialModel()
        if (credentialModel.count <= 0) {
            return
        }
        if (fillOnlyEmpty && (emailField.text.length > 0 || pwdField.text.length > 0)) {
            return
        }
        emailField.text = credentialModel.get(0).email
        pwdField.text = credentialModel.get(0).password
    }

    function saveCredential(email, password) {
        const normalizedEmail = String(email || "").trim()
        if (normalizedEmail.length <= 0 || String(password || "").length <= 0) {
            return
        }
        const records = parseCredentialCache().filter(function(item) {
            return String(item.email || "").trim().toLowerCase() !== normalizedEmail.toLowerCase()
        })
        records.unshift({ "email": normalizedEmail, "password": String(password || "") })
        if (credentialProvider) {
            credentialProvider.saveLoginCredential(normalizedEmail, String(password || ""))
        }
        credentialModel.clear()
        const savedRecords = records.slice(0, maxCachedCredentials)
        for (let i = 0; i < savedRecords.length; ++i) {
            credentialModel.append({ "email": savedRecords[i].email, "password": savedRecords[i].password })
        }
    }

    function applyCredential(index) {
        if (index < 0 || index >= credentialModel.count) {
            return
        }
        const record = credentialModel.get(index)
        emailField.text = record.email
        pwdField.text = record.password
        credentialDropdownOpen = false
        credentialDropdown.close()
        loginRoot.clearTipRequested()
    }

    Component.onCompleted: {
        revealProgress = 1.0
        loadLastCredential(false)
    }

    onVisibleChanged: {
        if (visible) {
            loadLastCredential(true)
            Qt.callLater(function() { credentialDropdown.reposition() })
        } else if (credentialDropdown.opened) {
            credentialDropdownOpen = false
            credentialDropdown.close()
        }
    }

    Connections {
        target: credentialProvider
        ignoreUnknownSignals: true
        function onLoginCredentialCacheChanged() {
            refreshCredentialModel()
            if (credentialDropdown.opened) {
                credentialDropdown.reposition()
            }
        }
    }

    ListModel {
        id: credentialModel
    }

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

    LoginCredentialDropdown {
        id: credentialDropdown
        hostItem: loginRoot
        anchorItem: emailInputBox
        backdrop: backdropLayer
        credentialModel: credentialModel
        surfaceFillColor: Qt.rgba(0.985, 0.992, 1.0, 0.48)
        surfaceStrokeColor: Qt.rgba(0.78, 0.86, 0.94, 0.66)
        itemBaseColor: Qt.rgba(1, 1, 1, 0.24)
        onClosed: credentialDropdownOpen = false
        onCredentialSelected: function(index) {
            loginRoot.applyCredential(index)
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

        LoginTopBar {
            id: topBar
            width: parent.width
            opacity: loginRoot.stageValue(0.02, 0.16)
            scale: 0.97 + 0.03 * opacity
            onSettingsClicked: loginRoot.clearTipRequested()
            onDragMoveRequested: {
                if (Window.window) {
                    Window.window.startSystemMove()
                }
            }
        }

        Item { width: parent.width; height: 12 }

        LoginAvatar {
            id: avatarSection
            anchors.horizontalCenter: parent.horizontalCenter
            backdrop: backdropLayer
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

        Item {
            width: parent.width
            height: 46
            opacity: loginRoot.stageValue(0.19, 0.18)
            scale: 0.97 + 0.03 * opacity

            Item {
                id: emailInputBox
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 46

                GlassTextField {
                    id: emailField
                    anchors.fill: parent
                    backdrop: backdropLayer
                    blurRadius: 28
                    cornerRadius: 11
                    fillColor: Qt.rgba(1, 1, 1, 0.10)
                    strokeColor: Qt.rgba(1, 1, 1, 0.28)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.16)
                    glowBottomColor: Qt.rgba(1, 1, 1, 0.03)
                    focusFillColor: Qt.rgba(1, 1, 1, 0.16)
                    focusStrokeColor: Qt.rgba(0.47, 0.71, 0.93, 0.64)
                    leftInset: 16
                    rightInset: 86
                    textPixelSize: 17
                    placeholderText: "输入邮箱"
                    inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                    maximumLength: 128
                    validator: emailInputValidator
                    onTextChanged: loginRoot.clearTipRequested()
                    onActiveFocusChanged: {
                        if (activeFocus && credentialDropdown.opened) {
                            credentialDropdown.close()
                        }
                    }
                }

                Rectangle {
                    id: credentialDropButton
                    z: 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    width: 72
                    height: 30
                    radius: 15
                    color: credentialDropArea.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                                      : credentialDropArea.containsMouse || credentialDropdownOpen
                                                        ? Qt.rgba(0.35, 0.61, 0.90, 0.16)
                                                        : Qt.rgba(1, 1, 1, 0.04)
                    border.width: 1
                    border.color: credentialDropdownOpen
                                  ? Qt.rgba(0.65, 0.84, 1.0, 0.78)
                                  : Qt.rgba(1, 1, 1, 0.12)

                    Behavior on color {
                        ColorAnimation {
                            duration: 120
                            easing.type: Easing.OutQuad
                        }
                    }

                    Behavior on border.color {
                        ColorAnimation {
                            duration: 120
                            easing.type: Easing.OutQuad
                        }
                    }

                    Row {
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            text: "最近"
                            color: credentialDropdownOpen ? "#2a5d7f" : "#496579"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Image {
                            width: 14
                            height: 14
                            source: "qrc:/icons/dropdown.png"
                            fillMode: Image.PreserveAspectFit
                            opacity: credentialDropArea.containsMouse || credentialDropdownOpen ? 0.95 : 0.72
                            rotation: credentialDropdownOpen ? 180 : 0

                            Behavior on rotation {
                                NumberAnimation {
                                    duration: 120
                                    easing.type: Easing.OutQuad
                                }
                            }

                            Behavior on opacity {
                                NumberAnimation {
                                    duration: 120
                                    easing.type: Easing.OutQuad
                                }
                            }
                        }
                    }
                }

                MouseArea {
                    id: credentialDropArea
                    z: 3
                    anchors.fill: credentialDropButton
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    enabled: true
                    onClicked: {
                        refreshCredentialModel()
                        credentialDropdownOpen = !credentialDropdown.opened
                        if (credentialDropdown.opened) {
                            credentialDropdown.close()
                        } else {
                            Qt.callLater(function() {
                                credentialDropdown.reposition()
                                credentialDropdown.open()
                            })
                        }
                    }
                }
            }
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
                fillColor: Qt.rgba(1, 1, 1, 0.10)
                strokeColor: Qt.rgba(1, 1, 1, 0.28)
                glowTopColor: Qt.rgba(1, 1, 1, 0.16)
                glowBottomColor: Qt.rgba(1, 1, 1, 0.03)
                focusFillColor: Qt.rgba(1, 1, 1, 0.16)
                focusStrokeColor: Qt.rgba(0.47, 0.71, 0.93, 0.64)
                leftInset: 16
                rightInset: 44
                textPixelSize: 17
                placeholderText: "输入密码"
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                maximumLength: 15
                validator: passwordInputValidator
                echoMode: loginRoot.pwdVisible ? TextInput.Normal : TextInput.Password
                onTextChanged: loginRoot.clearTipRequested()
                onActiveFocusChanged: {
                    if (activeFocus && credentialDropdown.opened) {
                        credentialDropdown.close()
                    }
                }
                onAccepted: loginBtn.triggerLogin()
            }

            Image {
                id: pwdEyeIcon
                z: 1
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 12
                width: 20
                height: 20
                source: loginRoot.pwdVisible ? "qrc:/res/visible.png" : "qrc:/res/unvisible.png"
                scale: pwdEyeArea.pressed ? 0.96 : (pwdEyeArea.containsMouse ? 1.02 : 1.0)

                Behavior on scale {
                    NumberAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }
            }

            Rectangle {
                anchors.centerIn: pwdEyeIcon
                width: 28
                height: 28
                radius: 14
                color: pwdEyeArea.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                          : pwdEyeArea.containsMouse ? Qt.rgba(0.35, 0.61, 0.90, 0.14)
                                                                    : "transparent"

                Behavior on color {
                    ColorAnimation {
                        duration: 110
                        easing.type: Easing.OutQuad
                    }
                }
            }

            MouseArea {
                id: pwdEyeArea
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 10
                width: 24
                height: 24
                hoverEnabled: true
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
                credentialDropdownOpen = false
                if (credentialDropdown.opened) {
                    credentialDropdown.close()
                }
                loginRoot.loginRequested(emailField.text.trim(), pwdField.text)
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
