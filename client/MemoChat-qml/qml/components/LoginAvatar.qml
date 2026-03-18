import QtQuick 2.15
import Qt5Compat.GraphicalEffects

Item {
    id: root

    property Item backdrop: null
    readonly property string logoText: "Memo"
    readonly property int logoPixelSize: 52
    readonly property real logoSpacing: 0.85

    width: 176
    height: 72

    Text {
        id: memoMask
        anchors.centerIn: parent
        text: root.logoText
        visible: false
        font.pixelSize: root.logoPixelSize
        font.weight: Font.DemiBold
        font.letterSpacing: root.logoSpacing
        renderType: Text.NativeRendering
    }

    ShaderEffectSource {
        id: blurSource
        anchors.fill: parent
        readonly property Item sourceBackdrop: root.backdrop !== null ? root.backdrop : root.parent
        sourceItem: sourceBackdrop !== null ? sourceBackdrop : null
        sourceRect: {
            if (!sourceBackdrop) {
                return Qt.rect(0, 0, root.width, root.height)
            }
            var p = root.mapToItem(sourceBackdrop, 0, 0)
            return Qt.rect(p.x, p.y, root.width, root.height)
        }
        live: true
        hideSource: true
        visible: false
    }

    Item {
        id: logoVisual
        anchors.fill: parent

        Text {
            id: memoShadow
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 1
            text: root.logoText
            color: Qt.rgba(0.35, 0.46, 0.62, 0.14)
            font.pixelSize: root.logoPixelSize
            font.weight: Font.DemiBold
            font.letterSpacing: root.logoSpacing
            renderType: Text.NativeRendering
        }

        FastBlur {
            id: blurredBackdrop
            anchors.fill: parent
            source: blurSource
            radius: 24
            transparentBorder: true
            visible: false
        }

        OpacityMask {
            id: glassText
            anchors.fill: parent
            source: blurredBackdrop
            maskSource: memoMask
            opacity: 0.93
        }

        Item {
            id: tintSource
            anchors.fill: parent
            visible: false

            LinearGradient {
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(width, height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(0.90, 0.95, 0.99, 0.44) }
                    GradientStop { position: 0.5; color: Qt.rgba(0.80, 0.88, 0.97, 0.36) }
                    GradientStop { position: 1.0; color: Qt.rgba(0.73, 0.82, 0.93, 0.40) }
                }
            }
        }

        OpacityMask {
            anchors.fill: parent
            source: tintSource
            maskSource: memoMask
            opacity: 0.52
        }

        Text {
            id: memoEdge
            anchors.centerIn: parent
            text: root.logoText
            color: Qt.rgba(1, 1, 1, 0.07)
            font.pixelSize: root.logoPixelSize
            font.weight: Font.DemiBold
            font.letterSpacing: root.logoSpacing
            renderType: Text.NativeRendering
            style: Text.Outline
            styleColor: Qt.rgba(1, 1, 1, 0.54)
        }

        Text {
            id: memoHighlight
            anchors.centerIn: parent
            text: root.logoText
            color: Qt.rgba(1, 1, 1, 0.13)
            font.pixelSize: root.logoPixelSize
            font.weight: Font.DemiBold
            font.letterSpacing: root.logoSpacing
            renderType: Text.NativeRendering
            scale: 1.002
        }
    }
}
