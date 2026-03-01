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
    scale: root.pressed ? 0.985 : 1.0

    Behavior on scale {
        NumberAnimation {
            duration: 110
            easing.type: Easing.OutQuad
        }
    }

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        blurRadius: root.hovering && root.ready ? 38 : 32
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

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(0.74, 0.89, 1.0, 0.75)
        opacity: root.ready ? (root.hovering ? 0.66 : 0.30) : 0.0

        Behavior on opacity {
            NumberAnimation {
                duration: 160
                easing.type: Easing.InOutQuad
            }
        }
    }

    Rectangle {
        id: sheen
        width: Math.max(42, root.width * 0.42)
        height: root.height * 1.9
        radius: width / 2
        y: -root.height * 0.45
        rotation: 17
        x: -width
        visible: root.ready
        opacity: root.hovering ? 0.42 : 0.0
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.0) }
            GradientStop { position: 0.52; color: Qt.rgba(1, 1, 1, 0.64) }
            GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.0) }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: 140
                easing.type: Easing.OutQuad
            }
        }
    }

    NumberAnimation {
        id: sheenAnim
        target: sheen
        property: "x"
        from: -sheen.width
        to: root.width + sheen.width
        duration: 1280
        easing.type: Easing.InOutQuad
        loops: Animation.Infinite
        running: root.hovering && root.ready && !root.busy
    }

    onHoveringChanged: {
        if (!hovering) {
            sheen.x = -sheen.width
        }
    }
    onBusyChanged: {
        if (busy) {
            sheen.x = -sheen.width
        }
    }

    Text {
        anchors.centerIn: parent
        text: root.busy ? root.busyText : root.idleText
        color: root.ready ? "#eaf5ff" : "#dfe5ec"
        font.pixelSize: 16
        font.weight: Font.DemiBold
        opacity: root.busy ? 0.9 : 1.0

        Behavior on opacity {
            NumberAnimation {
                duration: 120
                easing.type: Easing.OutQuad
            }
        }
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
