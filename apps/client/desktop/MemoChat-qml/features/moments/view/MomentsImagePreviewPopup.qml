pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: root

    property var mediaUrlResolver: null
    property string currentKey: ""

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: parent && parent.width ? parent.width - 40 : 680
    height: parent && parent.height ? parent.height - 40 : 500
    anchors.centerIn: Overlay.overlay
    background: Rectangle {
        anchors.fill: parent
        color: "#111111"
    }

    function openImage(key) {
        currentKey = key
        open()
    }

    function mediaUrl(key) {
        return key && mediaUrlResolver ? mediaUrlResolver(key) : ""
    }

    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: root.mediaUrl(root.currentKey)
        cache: false
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.close()
    }
}
