pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtWebEngine 1.10
import "qrc:/qml/components"

/**
 * R18EhentaiWebLoginDialog — off-the-record WebEngine login dialog.
 *
 * Security:
 *  - Cookie values never in QML state (controller holds them in C++).
 *  - Navigation restricted to E-Hentai, ExHentai, Cloudflare, hCaptcha.
 *  - Off-the-record profile from C++ controller.
 *  - Profile destroyed on every terminal path.
 */
Dialog {
    id: root

    required property string sourceId
    required property string gatewayUrl
    required property string authToken

    signal loginCompleted(bool success)

    title: "E-Hentai 网页登录"
    modal: true
    width: Math.min(1100, Screen.desktopAvailableWidth * 0.85)
    height: Math.min(780, Screen.desktopAvailableHeight * 0.85)
    standardButtons: Dialog.Cancel

    R18WebLoginController {
        id: loginController

        onCompleted: function(success) {
            root.loginCompleted(success)
            if (success) {
                Qt.callLater(function() { root.close() })
            }
        }
    }

    contentItem: Item {
        Column {
            anchors.fill: parent
            spacing: 0

            // Status bar — shows controller message, never cookie values
            Rectangle {
                width: parent.width
                height: 38
                color: loginController.status === "authenticated" ? "#e8f5e9"
                     : loginController.status === "failed"        ? "#ffebee"
                     : loginController.status === "collecting"    ? "#fff8e1"
                     : "#e3f2fd"

                Row {
                    anchors.centerIn: parent
                    spacing: 8

                    Rectangle {
                        visible: loginController.busy
                               && loginController.status !== "authenticated"
                               && loginController.status !== "failed"
                        width: 10; height: 10; radius: 5
                        color: "#1976d2"
                        RotationAnimation on rotation {
                            running: loginController.busy
                            from: 0; to: 360; duration: 900; loops: Animation.Infinite
                        }
                    }

                    Text {
                        text: loginController.message || "请在下方页面完成登录"
                        font.pixelSize: 13
                        color: loginController.status === "authenticated" ? "#388e3c"
                             : loginController.status === "failed"        ? "#c62828"
                             : "#1976d2"
                    }
                }
            }

            // Embedded WebEngineView using the off-the-record controller profile
            WebEngineView {
                id: webView
                width:  parent.width
                height: parent.height - 38
                profile: loginController.profile

                url: "https://forums.e-hentai.org/index.php?act=Login"

                onNavigationRequested: function(request) {
                    const host = request.url.host
                    const allowed = host.includes("e-hentai.org")
                                 || host.includes("exhentai.org")
                                 || host.includes("cloudflare.com")
                                 || host.includes("hcaptcha.com")
                    if (!allowed) {
                        console.warn("[R18 WebLogin] blocked navigation to:", host)
                        request.action = WebEngineNavigationRequest.IgnoreRequest
                    }
                }

                onLoadingChanged: function(info) {
                    if (info.status === WebEngineView.LoadFailedStatus) {
                        console.warn("[R18 WebLogin] load failed:", info.errorString)
                    }
                }

                settings.pluginsEnabled:           false
                settings.javascriptEnabled:        true
                settings.javascriptCanOpenWindows: false
                settings.localStorageEnabled:      false
                settings.autoLoadImages:           true
            }
        }
    }

    onAboutToShow: {
        loginController.startLogin(root.sourceId, root.gatewayUrl, root.authToken)
    }

    onRejected: {
        loginController.cancel()
    }

    onClosed: {
        loginController.cancel()
    }
}
