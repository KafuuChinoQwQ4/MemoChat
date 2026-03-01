import QtQuick 2.15

Item {
    id: root
    property Item backdrop: null
    property bool ready: false
    property bool busy: false
    property string idleText: "登录"
    property string busyText: "处理中..."
    signal triggered()

    property bool hovering: false
    property bool pressed: false

    width: 0
    height: 48
    clip: true

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        blurRadius: 32
        cornerRadius: 10
        fillColor: !root.ready ? Qt.rgba(0.60, 0.67, 0.74, 0.24)
                               : root.pressed ? Qt.rgba(0.25, 0.50, 0.72, 0.45)
                                              : root.hovering ? Qt.rgba(0.35, 0.62, 0.82, 0.40)
                                                              : Qt.rgba(0.45, 0.70, 0.88, 0.36)
        strokeColor: !root.ready ? Qt.rgba(1, 1, 1, 0.30)
                                 : Qt.rgba(1, 1, 1, 0.56)
        strokeWidth: 1
        glowTopColor: Qt.rgba(1, 1, 1, 0.24)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.04)
    }

    Text {
        anchors.centerIn: parent
        text: root.busy ? root.busyText : root.idleText
        color: root.ready ? "#eaf5ff" : "#dfe5ec"
        font.pixelSize: 16
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        enabled: !root.busy
        cursorShape: root.ready ? Qt.PointingHandCursor : Qt.ArrowCursor

        onEntered: root.hovering = true
        onExited: {
            root.hovering = false
            root.pressed = false
        }
        onPressed: {
            if (root.ready) {
                root.pressed = true
            }
        }
        onReleased: root.pressed = false
        onClicked: {
            if (root.ready) {
                root.triggered()
            }
        }
    }
}
