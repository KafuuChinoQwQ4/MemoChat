import QtQuick
import QtQuick.Controls
import QtWebEngine
import MemoChat.Core

/**
 * R18EhentaiWebLoginDialog — E-Hentai web login dialog with embedded WebEngineView.
 *
 * Opens E-Hentai forum login page in an off-the-record WebEngine profile.
 * The controller observes cookies in C++ and submits to backend automatically.
 */
Dialog {
    id: root

    property string sourceId: ""

    signal loginCompleted(bool success)

    title: "E-Hentai 网页登录"
    modal: true
    width: 1024
    height: 768
    standardButtons: Dialog.Cancel

    R18WebLoginController {
        id: controller

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

            // Status bar
            Rectangle {
                width: parent.width
                height: 40
                color: controller.status === "authenticated" ? "#e8f5e9"
                     : controller.status === "failed" ? "#ffebee"
                     : "#e3f2fd"

                Text {
                    anchors.centerIn: parent
                    text: controller.message || "请在下方页面完成登录"
                    font.pixelSize: 13
                    color: controller.status === "authenticated" ? "#388e3c"
                         : controller.status === "failed" ? "#c62828"
                         : "#1976d2"
                }
            }

            // WebEngineView
            WebEngineView {
                id: webView
                width: parent.width
                height: parent.height - 40
                profile: controller.profile

                url: "https://forums.e-hentai.org/index.php?act=Login"

                onLoadingChanged: function(loadRequest) {
                    if (loadRequest.status === WebEngineView.LoadFailedStatus) {
                        console.warn("R18 WebLogin: load failed:", loadRequest.errorString)
                    }
                }

                onNavigationRequested: function(request) {
                    const url = request.url.toString()
                    const host = request.url.host

                    // Allow E-Hentai, ExHentai, and Cloudflare domains only
                    const allowed = host.includes("e-hentai.org")
                                 || host.includes("exhentai.org")
                                 || host.includes("cloudflare.com")
                                 || host.includes("hcaptcha.com")

                    if (!allowed) {
                        console.warn("R18 WebLogin: blocked navigation to", host)
                        request.action = WebEngineNavigationRequest.IgnoreRequest
                    }
                }

                settings.pluginsEnabled: false
                settings.javascriptEnabled: true
                settings.javascriptCanOpenWindows: false
                settings.localStorageEnabled: false
                settings.autoLoadImages: true
            }
        }
    }

    onAccepted: {
        // No explicit accept action — controller handles completion
    }

    onRejected: {
        controller.cancel()
    }

    onAboutToShow: {
        controller.startLogin(sourceId)
    }

    onClosed: {
        controller.cancel()
    }
}
