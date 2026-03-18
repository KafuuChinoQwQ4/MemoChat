import QtQuick 2.15
import Qt5Compat.GraphicalEffects

Item {
    id: root

    property Item backdrop: null
    property bool blurEnabled: true
    property bool liveBlur: false
    property int blurRadius: 18
    property real cornerRadius: 10

    property color fillColor: Qt.rgba(1, 1, 1, 0.15)
    property color strokeColor: Qt.rgba(1, 1, 1, 0.50)
    property real strokeWidth: 1

    property color glowTopColor: Qt.rgba(1, 1, 1, 0.24)
    property color glowBottomColor: Qt.rgba(1, 1, 1, 0.06)
    readonly property bool effectActive: blurEnabled && visible && opacity > 0.01 && width > 1 && height > 1

    implicitWidth: 100
    implicitHeight: 38
    clip: true

    ShaderEffectSource {
        id: blurSource
        anchors.fill: parent
        readonly property Item sourceBackdrop: root.backdrop !== null ? root.backdrop : root
        sourceItem: root.effectActive && sourceBackdrop !== null ? sourceBackdrop : null
        sourceRect: {
            if (!sourceBackdrop || sourceBackdrop === root) {
                return Qt.rect(0, 0, root.width, root.height)
            }
            var p = root.mapToItem(sourceBackdrop, 0, 0)
            return Qt.rect(p.x, p.y, root.width, root.height)
        }
        live: root.effectActive && root.liveBlur
        hideSource: true
        visible: false
    }

    Item {
        id: blurLayer
        anchors.fill: parent
        visible: root.effectActive

        FastBlur {
            id: blurEffect
            anchors.fill: parent
            source: root.effectActive ? blurSource : null
            radius: root.blurRadius
            transparentBorder: false
            visible: false
        }

        OpacityMask {
            anchors.fill: parent
            source: blurEffect
            maskSource: Rectangle {
                width: blurLayer.width
                height: blurLayer.height
                radius: root.cornerRadius
                color: "black"
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: root.fillColor
        border.color: root.strokeColor
        border.width: root.strokeWidth

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

    Behavior on blurRadius {
        NumberAnimation {
            duration: 120
            easing.type: Easing.InOutQuad
        }
    }
}
