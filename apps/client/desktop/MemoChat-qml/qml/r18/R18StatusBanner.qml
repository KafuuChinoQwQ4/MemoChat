import QtQuick 2.15

Item {
    id: root

    property string statusText: ""
    property string errorText: ""
    property bool statusVisible: statusText.length > 0 && errorText.length === 0
    property color textColor: "#4e6072"

    implicitHeight: Math.max(statusBanner.height, errorBanner.height)

    Rectangle {
        id: statusBanner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: statusLabel.visible ? statusLabel.implicitHeight + 18 : 0
        radius: 8
        color: Qt.rgba(0.54, 0.70, 0.93, 0.24)
        border.color: Qt.rgba(0.28, 0.45, 0.67, 0.28)
        visible: root.statusVisible

        Text {
            id: statusLabel
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            visible: root.statusVisible
            text: root.statusText
            color: root.textColor
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }
    }

    Rectangle {
        id: errorBanner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: errorLabel.visible ? errorLabel.implicitHeight + 18 : 0
        radius: 8
        color: Qt.rgba(0.92, 0.62, 0.68, 0.26)
        border.color: Qt.rgba(0.54, 0.18, 0.24, 0.28)
        visible: root.errorText.length > 0

        Text {
            id: errorLabel
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            visible: root.errorText.length > 0
            text: root.errorText
            color: "#6f1f2c"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }
    }
}
