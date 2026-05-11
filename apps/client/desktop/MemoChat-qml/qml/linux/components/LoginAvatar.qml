import QtQuick 2.15

Item {
    id: root

    property Item backdrop: null
    readonly property string logoText: "Memo"
    readonly property int logoPixelSize: 52
    readonly property real logoSpacing: 0.85

    width: 176
    height: 72

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
