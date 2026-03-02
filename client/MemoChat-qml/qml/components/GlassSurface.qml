import QtQuick 2.15
import Qt5Compat.GraphicalEffects

Item {
    id: root

    required property Item backdrop
    property int blurRadius: 26
    property real cornerRadius: 10

    property color fillColor: Qt.rgba(1, 1, 1, 0.15)
    property color strokeColor: Qt.rgba(1, 1, 1, 0.50)
    property real strokeWidth: 1

    property color glowTopColor: Qt.rgba(1, 1, 1, 0.24)
    property color glowBottomColor: Qt.rgba(1, 1, 1, 0.06)

    implicitWidth: 100
    implicitHeight: 38
    clip: true
    layer.enabled: true
    layer.smooth: true
    layer.effect: OpacityMask {
        maskSource: Rectangle {
            width: root.width
            height: root.height
            radius: root.cornerRadius
            color: "black"
        }
    }

    ShaderEffectSource {
        id: blurSource
        anchors.fill: parent
        sourceItem: root.backdrop
        sourceRect: {
            var p = root.mapToItem(root.backdrop, 0, 0)
            return Qt.rect(p.x, p.y, root.width, root.height)
        }
        live: true
        hideSource: true
        visible: false
    }

    Item {
        id: blurLayer
        anchors.fill: parent

        FastBlur {
            id: blurEffect
            anchors.fill: parent
            source: blurSource
            radius: root.blurRadius
            transparentBorder: true
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
            duration: 220
            easing.type: Easing.InOutQuad
        }
    }
}
