import QtQuick 2.15
import QtQuick.Effects

Item {
    id: root

    property Item backdrop: null
    readonly property string logoText: "Memo"
    readonly property int logoPixelSize: 52
    readonly property real logoSpacing: 0.85
    readonly property bool hasBackdrop: backdrop !== null

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
        readonly property Item sourceBackdrop: root.backdrop
        sourceItem: root.hasBackdrop && sourceBackdrop !== null ? sourceBackdrop : null
        sourceRect: {
            if (!sourceBackdrop) {
                return Qt.rect(0, 0, root.width, root.height)
            }
            var p = root.mapToItem(sourceBackdrop, 0, 0)
            return Qt.rect(p.x, p.y, root.width, root.height)
        }
        live: root.hasBackdrop
        hideSource: true
        visible: false
    }

    ShaderEffectSource {
        id: maskSource
        anchors.fill: parent
        sourceItem: memoMask
        sourceRect: Qt.rect(0, 0, root.width, root.height)
        live: false
        hideSource: true
        visible: false
    }

    Item {
        id: tintSource
        anchors.fill: parent
        visible: false

        Rectangle {
            anchors.fill: parent
            radius: 18
            antialiasing: true
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0.90, 0.95, 0.99, 0.44) }
                GradientStop { position: 0.5; color: Qt.rgba(0.80, 0.88, 0.97, 0.36) }
                GradientStop { position: 1.0; color: Qt.rgba(0.73, 0.82, 0.93, 0.40) }
            }
        }
    }

    MultiEffect {
        anchors.fill: parent
        source: root.hasBackdrop ? blurSource : tintSource
        visible: true
        blurEnabled: root.hasBackdrop
        blur: root.hasBackdrop ? 0.58 : 0.0
        blurMax: 48
        maskEnabled: true
        maskSource: maskSource
        maskSpreadAtMin: 0.05
        maskSpreadAtMax: 0.12
        autoPaddingEnabled: false
        opacity: 0.98
    }

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
