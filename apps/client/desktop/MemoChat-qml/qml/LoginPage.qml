import QtQuick 2.15
import QtQuick.Controls 2.15
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
    property real credentialPopupScale: 1.0

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

    function maskedPassword(password) {
        const length = Math.max(0, String(password || "").length)
        if (length <= 0) {
            return "未保存密码"
        }
        return "\u2022".repeat(Math.min(length, 10))
    }

    function credentialPanelHeight() {
        const listHeight = Math.min(Math.max(credentialModel.count, 1) * 56, 224)
        return 58 + listHeight + 14
    }

    function positionCredentialPopup() {
        if (!credentialPopup.parent || !emailInputBox) {
            return
        }
        const overlayPoint = emailInputBox.mapToItem(credentialPopup.parent, 0, emailInputBox.height)
        if (!isFinite(overlayPoint.x) || !isFinite(overlayPoint.y)) {
            return
        }

        const horizontalMargin = 12
        const popupWidth = Math.min(Math.max(emailInputBox.width, 280), loginRoot.width - horizontalMargin * 2)
        const maxX = Math.max(horizontalMargin, loginRoot.width - popupWidth - horizontalMargin)
        credentialPopup.width = popupWidth
        credentialPopup.x = Math.max(horizontalMargin, Math.min(overlayPoint.x, maxX))
        credentialPopup.y = overlayPoint.y + 10
    }

    function applyCredential(index) {
        if (index < 0 || index >= credentialModel.count) {
            return
        }
        const record = credentialModel.get(index)
        emailField.text = record.email
        pwdField.text = record.password
        credentialDropdownOpen = false
        credentialPopup.close()
        loginRoot.clearTipRequested()
    }

    Component.onCompleted: {
        revealProgress = 1.0
        loadLastCredential(false)
    }

    onVisibleChanged: {
        if (visible) {
            loadLastCredential(true)
            Qt.callLater(function() { positionCredentialPopup() })
        } else if (credentialPopup.opened) {
            credentialDropdownOpen = false
            credentialPopup.close()
        }
    }

    Connections {
        target: credentialProvider
        ignoreUnknownSignals: true
        function onLoginCredentialCacheChanged() {
            refreshCredentialModel()
            if (credentialPopup.opened) {
                positionCredentialPopup()
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

    Popup {
        id: credentialPopup
        parent: Overlay.overlay
        z: 1100
        width: 320
        height: credentialPanelHeight()
        padding: 0
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        opacity: 1.0
        scale: loginRoot.credentialPopupScale
        transformOrigin: Item.Top

        onAboutToShow: {
            loginRoot.credentialPopupScale = 0.94
            positionCredentialPopup()
        }
        onOpened: Qt.callLater(function() { positionCredentialPopup() })
        onClosed: credentialDropdownOpen = false

        enter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 0.0
                    to: 1.0
                    duration: 150
                    easing.type: Easing.OutQuad
                }
                NumberAnimation {
                    target: loginRoot
                    property: "credentialPopupScale"
                    from: 0.94
                    to: 1.0
                    duration: 180
                    easing.type: Easing.OutCubic
                }
            }
        }

        exit: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "opacity"
                    from: 1.0
                    to: 0.0
                    duration: 110
                    easing.type: Easing.InQuad
                }
                NumberAnimation {
                    target: loginRoot
                    property: "credentialPopupScale"
                    from: 1.0
                    to: 0.97
                    duration: 110
                    easing.type: Easing.InQuad
                }
            }
        }

        background: GlassSurface {
            anchors.fill: parent
            backdrop: backdropLayer
            blurRadius: 28
            cornerRadius: 18
            fillColor: Qt.rgba(0.985, 0.992, 1.0, 0.48)
            strokeColor: Qt.rgba(0.78, 0.86, 0.94, 0.66)
            strokeWidth: 1
            glowTopColor: Qt.rgba(1, 1, 1, 0.42)
            glowBottomColor: Qt.rgba(0.70, 0.80, 0.92, 0.10)
        }

        contentItem: Item {
            implicitWidth: credentialPopup.width
            implicitHeight: credentialPopup.height

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                height: 48
                radius: 14
                color: Qt.rgba(1, 1, 1, 0.30)
                border.width: 1
                border.color: Qt.rgba(0.80, 0.87, 0.95, 0.58)

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 14
                    spacing: 10

                    Rectangle {
                        width: 28
                        height: 28
                        radius: 14
                        color: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                        border.width: 1
                        border.color: Qt.rgba(0.71, 0.86, 1.0, 0.42)

                        Text {
                            anchors.centerIn: parent
                            text: "\u2198"
                            color: "#33698a"
                            font.pixelSize: 16
                            font.bold: true
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        Text {
                            text: "最近登录"
                            color: "#233244"
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Text {
                            text: credentialModel.count > 0
                                  ? "选择一个历史账号快速填充"
                                  : "登录成功后会自动保存在这里"
                            color: "#6a7d90"
                            font.pixelSize: 11
                        }
                    }
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 12
                    width: 46
                    height: 26
                    radius: 13
                    color: Qt.rgba(0.24, 0.46, 0.66, 0.10)
                    border.width: 1
                    border.color: Qt.rgba(0.60, 0.78, 0.95, 0.46)

                    Text {
                        anchors.centerIn: parent
                        text: credentialModel.count > 0 ? credentialModel.count : 0
                        color: "#2b5876"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
            }

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 64
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10

                Column {
                    anchors.centerIn: parent
                    width: parent.width - 28
                    spacing: 8
                    visible: credentialModel.count <= 0

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 54
                        height: 54
                        radius: 27
                        color: Qt.rgba(0.35, 0.61, 0.90, 0.12)
                        border.width: 1
                        border.color: Qt.rgba(0.70, 0.84, 0.98, 0.34)

                        Text {
                            anchors.centerIn: parent
                            text: "@"
                            color: "#49708f"
                            font.pixelSize: 22
                            font.bold: true
                        }
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: "暂无历史账号"
                        color: "#274056"
                        font.pixelSize: 15
                        font.bold: true
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: "完成一次登录后，这里会展示最近使用过的邮箱。"
                        color: "#72859a"
                        font.pixelSize: 12
                    }
                }

                ListView {
                    id: credentialList
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    clip: true
                    visible: credentialModel.count > 0
                    spacing: 8
                    model: credentialModel
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: GlassScrollBar { }

                    delegate: Item {
                        width: credentialList.width
                        height: 56

                        Rectangle {
                            anchors.fill: parent
                            radius: 14
                            color: credentialMouse.containsMouse
                                   ? Qt.rgba(0.35, 0.61, 0.90, 0.16)
                                   : Qt.rgba(1, 1, 1, 0.24)
                            border.width: 1
                            border.color: credentialMouse.containsMouse
                                          ? Qt.rgba(0.71, 0.86, 1.0, 0.50)
                                          : Qt.rgba(0.83, 0.89, 0.95, 0.56)

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
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            width: 34
                            height: 34
                            radius: 17
                            color: Qt.rgba(0.31, 0.55, 0.78, 0.14)
                            border.width: 1
                            border.color: Qt.rgba(0.71, 0.86, 1.0, 0.28)

                            Text {
                                anchors.centerIn: parent
                                text: email && email.length > 0 ? email.charAt(0).toUpperCase() : "@"
                                color: "#2f5d7f"
                                font.pixelSize: 15
                                font.bold: true
                            }
                        }

                        Column {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 58
                            anchors.rightMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3

                            Text {
                                text: email
                                color: "#203246"
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: "密码 " + loginRoot.maskedPassword(password)
                                color: "#6b7f92"
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            id: credentialMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: loginRoot.applyCredential(index)
                        }
                    }
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

        LoginTopBar {
            id: topBar
            width: parent.width
            opacity: loginRoot.stageValue(0.02, 0.16)
            scale: 0.97 + 0.03 * opacity
            onSettingsClicked: loginRoot.clearTipRequested()
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
                        if (activeFocus && credentialPopup.opened) {
                            credentialPopup.close()
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
                        credentialDropdownOpen = !credentialPopup.opened
                        if (credentialPopup.opened) {
                            credentialPopup.close()
                        } else {
                            Qt.callLater(function() {
                                positionCredentialPopup()
                                credentialPopup.open()
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
                    if (activeFocus && credentialPopup.opened) {
                        credentialPopup.close()
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
                if (credentialPopup.opened) {
                    credentialPopup.close()
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
