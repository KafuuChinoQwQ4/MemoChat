import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: root
    property Item anchorItem: null
    property Item hostItem: null
    property Item backdrop: null
    property Item popupHost: Overlay.overlay ? Overlay.overlay : (root.hostItem ? root.hostItem : parent)

    signal registerClicked()
    signal resetClicked()

    parent: popupHost
    z: 1000
    width: 108
    height: 94
    padding: 8
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    x: 10
    y: 10

    function reposition() {
        if (!root.anchorItem) {
            return
        }

        var host = root.popupHost ? root.popupHost : (root.hostItem ? root.hostItem : parent)
        var p = root.anchorItem.mapToItem(host, 0, root.anchorItem.height)
        if (!isFinite(p.x) || !isFinite(p.y)) {
            return
        }

        var xPos = p.x - width / 2 + root.anchorItem.width / 2
        var yPos = p.y + 12
        var maxX = Math.max(0, host.width - width - 10)
        var maxY = Math.max(0, host.height - height - 10)
        x = Math.max(10, Math.min(xPos, maxX))
        y = Math.max(10, Math.min(yPos, maxY))
    }

    onAboutToShow: reposition()
    onOpened: Qt.callLater(function() { reposition() })

    background: GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        blurRadius: 24
        cornerRadius: 10
        fillColor: Qt.rgba(1, 1, 1, 0.20)
        strokeColor: Qt.rgba(1, 1, 1, 0.56)
        strokeWidth: 1
        glowTopColor: Qt.rgba(1, 1, 1, 0.24)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.05)
    }

    Column {
        anchors.fill: parent
        spacing: 6

        GlassButton {
            width: parent.width
            height: 34
            cornerRadius: 7
            text: "注册账号"
            onClicked: {
                root.close()
                root.registerClicked()
            }
        }

        GlassButton {
            width: parent.width
            height: 34
            cornerRadius: 7
            text: "忘记密码"
            onClicked: {
                root.close()
                root.resetClicked()
            }
        }
    }
}
