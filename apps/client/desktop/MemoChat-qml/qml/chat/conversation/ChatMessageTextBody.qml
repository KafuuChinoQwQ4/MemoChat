import QtQuick 2.15

Text {
    id: root

    property string messageText: ""
    property real maxWidth: 240

    text: root.messageText
    width: Math.min(root.maxWidth, implicitWidth)
    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
    color: "#213045"
    font.pixelSize: 14
}
