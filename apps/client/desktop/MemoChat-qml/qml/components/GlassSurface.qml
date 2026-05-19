import QtQuick 2.15
import QtQuick.Effects

Item {
    id: root

    property Item backdrop: null
    property bool blurEnabled: Qt.platform.os !== "linux"
    property bool liveBlur: false
    property int blurRadius: 18
    property real cornerRadius: 10

    property color fillColor: Qt.rgba(1, 1, 1, 0.15)
    property color strokeColor: Qt.rgba(1, 1, 1, 0.50)
    property real strokeWidth: 1

    property color glowTopColor: Qt.rgba(1, 1, 1, 0.24)
    property color glowBottomColor: Qt.rgba(1, 1, 1, 0.06)
    readonly property bool effectActive: blurEnabled && backdrop !== null && visible && opacity > 0.01 && width > 1 && height > 1

    implicitWidth: 100
    implicitHeight: 38
    clip: true

    Rectangle {
        id: maskShape
        anchors.fill: parent
        radius: root.cornerRadius
        color: "white"
        antialiasing: true
        visible: false
    }

    ShaderEffectSource {
        id: blurSource
        anchors.fill: parent
        readonly property Item sourceBackdrop: root.backdrop
        sourceItem: root.effectActive && sourceBackdrop !== null ? sourceBackdrop : null
        sourceRect: {
            if (!sourceBackdrop) {
                return Qt.rect(0, 0, root.width, root.height)
            }
            var p = root.mapToItem(sourceBackdrop, 0, 0)
            return Qt.rect(p.x, p.y, root.width, root.height)
        }
        live: root.effectActive && root.liveBlur
        hideSource: true
        visible: false
    }

    ShaderEffectSource {
        id: maskSource
        anchors.fill: parent
        sourceItem: root.effectActive ? maskShape : null
        sourceRect: Qt.rect(0, 0, root.width, root.height)
        live: false
        hideSource: true
        visible: false
    }

    MultiEffect {
        anchors.fill: parent
        source: root.effectActive ? blurSource : null
        visible: root.effectActive
        blurEnabled: root.effectActive
        blur: Math.max(0.0, Math.min(1.0, root.blurRadius / 48.0))
        blurMax: 48
        maskEnabled: true
        maskSource: maskSource
        maskSpreadAtMin: 0.08
        maskSpreadAtMax: 0.14
        autoPaddingEnabled: false
    }

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: root.fillColor
        border.color: root.strokeColor
        border.width: root.strokeWidth
        antialiasing: true

        Behavior on color {
            ColorAnimation {
                duration: 180
                easing.type: Easing.InOutQuad
            }
        }
        Behavior on border.color {
            ColorAnimation {
                duration: 180
                easing.type: Easing.InOutQuad
            }
        }
        Behavior on border.width {
            NumberAnimation {
                duration: 160
                easing.type: Easing.InOutQuad
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        antialiasing: true
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.glowTopColor }
            GradientStop { position: 0.46; color: Qt.rgba(1, 1, 1, 0) }
            GradientStop { position: 1.0; color: root.glowBottomColor }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: Math.max(1, parent.height * 0.42)
        radius: root.cornerRadius
        antialiasing: true
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.18) }
            GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0) }
        }
        opacity: root.blurEnabled ? 0.72 : 0.42
    }

    Behavior on blurRadius {
        NumberAnimation {
            duration: 120
            easing.type: Easing.InOutQuad
        }
    }
}
