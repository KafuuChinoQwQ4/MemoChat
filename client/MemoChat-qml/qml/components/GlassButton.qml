import QtQuick 2.15

Item {
    id: root

    property string text: ""
    property int textPixelSize: 15
    property color textColor: "#5f6673"
    property color disabledTextColor: "#9aa1ad"

    property color normalColor: Qt.rgba(0, 0, 0, 0.07)
    property color hoverColor: Qt.rgba(0, 0, 0, 0.12)
    property color pressedColor: Qt.rgba(0, 0, 0, 0.20)
    property color disabledColor: Qt.rgba(0, 0, 0, 0.05)

    property real cornerRadius: 10
    property bool hovering: false
    property bool pressed: false

    signal clicked()

    implicitWidth: 120
    implicitHeight: 38

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: !root.enabled ? root.disabledColor
                            : root.pressed ? root.pressedColor
                                           : root.hovering ? root.hoverColor
                                                           : root.normalColor
        border.width: 0
    }

    Text {
        anchors.centerIn: parent
        text: root.text
        color: root.enabled ? root.textColor : root.disabledTextColor
        font.pixelSize: root.textPixelSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onEntered: root.hovering = true
        onExited: {
            root.hovering = false
            root.pressed = false
        }
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onCanceled: root.pressed = false
        onClicked: root.clicked()
    }
}
