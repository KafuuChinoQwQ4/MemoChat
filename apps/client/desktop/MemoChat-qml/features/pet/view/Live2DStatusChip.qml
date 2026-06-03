import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root

    property string text: ""
    property color colorBase: Qt.rgba(0.32, 0.56, 0.86, 1.0)
    property color textColor: "#4e5d74"

    implicitWidth: chipText.implicitWidth + 18
    implicitHeight: 24
    radius: 8
    color: Qt.rgba(colorBase.r, colorBase.g, colorBase.b, 0.15)
    border.color: Qt.rgba(colorBase.r, colorBase.g, colorBase.b, 0.26)

    Label {
        id: chipText
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        text: root.text
        color: root.textColor
        font.pixelSize: 11
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
