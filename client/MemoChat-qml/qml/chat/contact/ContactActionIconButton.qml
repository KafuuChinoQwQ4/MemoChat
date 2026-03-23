import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    // 圆形点击区 = iconSize；有文案时总高 = 圆 + 标签行
    implicitWidth: labelText.length > 0 ? Math.max(iconSize + 8, 52) : iconSize + 8
    implicitHeight: labelText.length > 0 ? (iconSize + 8) + labelRowHeight : iconSize + 8

    property int iconSize: 32
    property int labelRowHeight: 16
    property string labelText: ""

    property string normalSource: ""
    property string hoverSource: ""
    property string pressedSource: ""
    property bool enableScaleFeedback: true
    property real hoverScale: 1.02
    property real pressedScale: 0.96

    signal clicked()
    scale: {
        if (!root.enabled || !root.enableScaleFeedback) {
            return 1.0
        }
        if (mouse.pressed) {
            return root.pressedScale
        }
        if (mouse.containsMouse) {
            return root.hoverScale
        }
        return 1.0
    }

    Behavior on scale {
        NumberAnimation {
            duration: 110
            easing.type: Easing.OutQuad
        }
    }

    Rectangle {
        id: hitDisk
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: root.iconSize + 8
        height: root.iconSize + 8
        radius: width / 2
        color: !root.enabled ? Qt.rgba(1, 1, 1, 0.08)
                             : mouse.pressed ? Qt.rgba(0.55, 0.72, 0.94, 0.34)
                                             : mouse.containsMouse ? Qt.rgba(0.55, 0.72, 0.94, 0.24)
                                                                  : Qt.rgba(1, 1, 1, 0.22)
        border.color: Qt.rgba(1, 1, 1, 0.38)

        Behavior on color {
            ColorAnimation {
                duration: 120
                easing.type: Easing.OutQuad
            }
        }

        Image {
            anchors.centerIn: parent
            width: Math.max(12, root.iconSize - 6)
            height: Math.max(12, root.iconSize - 6)
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            source: {
                if (!root.enabled) {
                    return root.normalSource
                }
                if (mouse.pressed && root.pressedSource.length > 0) {
                    return root.pressedSource
                }
                if (mouse.containsMouse && root.hoverSource.length > 0) {
                    return root.hoverSource
                }
                return root.normalSource
            }
            opacity: root.enabled ? 1.0 : 0.45
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: hitDisk
        enabled: root.enabled
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.clicked()
    }

    Label {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: hitDisk.bottom
        anchors.topMargin: 4
        text: root.labelText
        color: "#3d4f66"
        font.pixelSize: 11
        visible: root.labelText.length > 0
    }
}
