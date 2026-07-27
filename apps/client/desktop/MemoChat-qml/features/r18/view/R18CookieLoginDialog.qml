pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "qrc:/qml/components"

/**
 * R18CookieLoginDialog — labeled cookie-value input for nhentai / hanime1.
 *
 * QML only holds the *name* labels (read-only) and forwards *values* to the
 * C++ controller, which assembles the cookie header and never returns it to QML.
 *
 * Usage:
 *   R18CookieLoginDialog {
 *     sourceId:   "nhentai.official"
 *     sourceName: "nHentai"
 *     cookieFields: [
 *       { name: "sessionid",  hint: "Django session ID", required: true  },
 *       { name: "csrftoken",  hint: "CSRF token",        required: false },
 *     ]
 *     gatewayUrl: backend.gatewayUrl
 *     authToken:  session.token
 *   }
 */
Dialog {
    id: root

    required property string sourceId
    required property string sourceName
    required property var    cookieFields   // array of { name, hint, required }
    required property string gatewayUrl
    required property string authToken

    signal loginCompleted(bool success)

    title: root.sourceName + " Cookie 登录"
    modal: true
    width:  Math.min(520, Screen.desktopAvailableWidth  * 0.9)
    height: Math.min(460, Screen.desktopAvailableHeight * 0.8)
    standardButtons: Dialog.Cancel

    R18CookieLoginController {
        id: cookieController

        onCompleted: function(success) {
            root.loginCompleted(success)
            if (success)
                Qt.callLater(function() { root.close() })
        }
    }

    contentItem: ColumnLayout {
        spacing: 14

        // Status / help area
        Rectangle {
            Layout.fillWidth: true
            height:           38
            radius:           6
            color: cookieController.status === "authenticated" ? "#e8f5e9"
                 : cookieController.status === "failed"        ? "#ffebee"
                 : cookieController.status === "importing"     ? "#fff8e1"
                 : "#e3f2fd"

            Text {
                anchors.centerIn: parent
                text: cookieController.message.length > 0
                    ? cookieController.message
                    : qsTr("在浏览器中登录 %1，然后从 F12 → Application → Cookies 复制各字段的值").arg(root.sourceName)
                font.pixelSize: 12
                color: cookieController.status === "authenticated" ? "#388e3c"
                     : cookieController.status === "failed"        ? "#c62828"
                     : "#1976d2"
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                width: parent.width - 20
            }
        }

        // Cookie field rows
        Repeater {
            id: fieldRepeater
            model: root.cookieFields

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // Cookie name — read-only chip
                Rectangle {
                    width:  140
                    height: 34
                    radius: 6
                    color:  "#f0f4f8"
                    border.color: "#c8d0db"

                    Text {
                        anchors.centerIn: parent
                        text: modelData.name
                        font.pixelSize: 12
                        font.family:    "monospace"
                        font.bold:      true
                        color:          "#263241"
                    }
                }

                Text {
                    text:           "="
                    font.pixelSize: 16
                    color:          "#8493a3"
                }

                // Value input — password field so value is masked
                TextField {
                    id:                 valueField
                    Layout.fillWidth:   true
                    height:             34
                    echoMode:           TextInput.Password
                    placeholderText:    modelData.hint ?? qsTr("粘贴 value")
                    font.pixelSize:     12
                    enabled:            !cookieController.busy
                    background: Rectangle {
                        radius:       6
                        color:        "#f8fafc"
                        border.color: valueField.activeFocus ? "#6397d6" : "#c8d0db"
                    }
                }
            }
        }

        // Submit / clear buttons
        RowLayout {
            spacing: 8

            Button {
                text:    cookieController.busy ? qsTr("导入中…") : qsTr("导入 Cookie")
                enabled: !cookieController.busy
                onClicked: {
                    // Collect values from field repeater and forward to controller.
                    var values = {}
                    for (var i = 0; i < fieldRepeater.count; ++i) {
                        var row  = fieldRepeater.itemAt(i)
                        if (!row) continue
                        // Find the TextField by walking children
                        var tf = null
                        for (var j = 0; j < row.children.length; ++j) {
                            if (row.children[j] instanceof TextField)
                                tf = row.children[j]
                        }
                        if (tf && tf.text.trim().length > 0)
                            values[root.cookieFields[i].name] = tf.text.trim()
                    }
                    cookieController.submitCookies(values)
                }
            }

            Button {
                text:    qsTr("重置")
                enabled: !cookieController.busy
                onClicked: {
                    cookieController.reset()
                    for (var i = 0; i < fieldRepeater.count; ++i) {
                        var row = fieldRepeater.itemAt(i)
                        if (!row) continue
                        for (var j = 0; j < row.children.length; ++j) {
                            if (row.children[j] instanceof TextField)
                                row.children[j].text = ""
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    onAboutToShow: {
        cookieController.init(root.sourceId, root.gatewayUrl, root.authToken)
    }

    onRejected: {
        cookieController.cancel()
    }

    onClosed: {
        cookieController.cancel()
    }
}
