import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root

    required property Item backdrop

    property alias text: input.text
    property alias readOnly: input.readOnly
    property string placeholderText: ""
    property int echoMode: TextInput.Normal
    property int textPixelSize: 15
    property int textHorizontalAlignment: TextInput.AlignHCenter
    property int textVerticalAlignment: TextInput.AlignVCenter
    property bool enableMouseSelection: true
    property int leftInset: 12
    property int rightInset: 12

    property int blurRadius: 26
    property real cornerRadius: 10
    property color fillColor: Qt.rgba(1, 1, 1, 0.15)
    property color strokeColor: Qt.rgba(1, 1, 1, 0.50)
    property real strokeWidth: 1
    property color glowTopColor: Qt.rgba(1, 1, 1, 0.24)
    property color glowBottomColor: Qt.rgba(1, 1, 1, 0.06)

    signal accepted()

    implicitWidth: 140
    implicitHeight: 38
    clip: true

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop
        blurRadius: root.blurRadius
        cornerRadius: root.cornerRadius
        fillColor: root.fillColor
        strokeColor: root.strokeColor
        strokeWidth: root.strokeWidth
        glowTopColor: root.glowTopColor
        glowBottomColor: root.glowBottomColor
    }

    TextField {
        id: input
        anchors.fill: parent
        anchors.leftMargin: root.leftInset
        anchors.rightMargin: root.rightInset
        placeholderText: root.placeholderText
        font.pixelSize: root.textPixelSize
        horizontalAlignment: root.textHorizontalAlignment
        verticalAlignment: root.textVerticalAlignment
        echoMode: root.echoMode
        selectByMouse: root.enableMouseSelection
        background: Item { }
        onAccepted: root.accepted()
    }
}
