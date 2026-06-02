import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root

    property var auth: null

    signal switchToRegisterRequested()
    signal switchToResetRequested()

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 320)
        spacing: 12

        TextField {
            id: emailField
            width: parent.width
            placeholderText: "Email"
            enabled: !root.auth || !root.auth.busy
        }

        TextField {
            id: passwordField
            width: parent.width
            placeholderText: "Password"
            echoMode: TextInput.Password
            enabled: !root.auth || !root.auth.busy
        }

        Label {
            width: parent.width
            visible: root.auth && root.auth.tipText.length > 0
            text: root.auth ? root.auth.tipText : ""
            color: root.auth && root.auth.tipError ? "#d64545" : "#2a7f62"
            wrapMode: Text.Wrap
        }

        Button {
            width: parent.width
            text: root.auth && root.auth.busy ? "Logging in..." : "Login"
            enabled: root.auth && !root.auth.busy
            onClicked: root.auth.login(emailField.text, passwordField.text)
        }

        Row {
            spacing: 8

            Button {
                text: "Register"
                onClicked: root.switchToRegisterRequested()
            }

            Button {
                text: "Reset"
                onClicked: root.switchToResetRequested()
            }
        }
    }
}

