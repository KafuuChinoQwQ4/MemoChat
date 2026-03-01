import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root

    required property Item backdrop

    property alias text: input.text
    property alias readOnly: input.readOnly
    property alias validator: input.validator
    property alias inputMethodHints: input.inputMethodHints
    property alias maximumLength: input.maximumLength
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
    property bool focusAccentEnabled: true
    property color focusFillColor: Qt.rgba(1, 1, 1, 0.20)
    property color focusStrokeColor: Qt.rgba(0.47, 0.71, 0.93, 0.84)
    property real focusStrokeWidth: 1.2
    property int focusBlurRadius: 34

    signal accepted()

    implicitWidth: 140
    implicitHeight: 38
    clip: true

    GlassSurface {
        id: fieldSurface
        anchors.fill: parent
        backdrop: root.backdrop
        blurRadius: root.focusAccentEnabled && input.activeFocus ? root.focusBlurRadius : root.blurRadius
        cornerRadius: root.cornerRadius
        fillColor: root.focusAccentEnabled && input.activeFocus ? root.focusFillColor : root.fillColor
        strokeColor: root.focusAccentEnabled && input.activeFocus ? root.focusStrokeColor : root.strokeColor
        strokeWidth: root.focusAccentEnabled && input.activeFocus ? root.focusStrokeWidth : root.strokeWidth
        glowTopColor: root.glowTopColor
        glowBottomColor: root.glowBottomColor
    }

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: "transparent"
        border.width: 1
        border.color: Qt.rgba(0.65, 0.84, 1.0, 0.7)
        opacity: root.focusAccentEnabled && input.activeFocus ? 0.55 : 0.0

        Behavior on opacity {
            NumberAnimation {
                duration: 160
                easing.type: Easing.InOutQuad
            }
        }
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
