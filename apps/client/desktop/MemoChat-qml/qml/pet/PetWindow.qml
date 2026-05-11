import QtQuick 2.15
import QtQuick.Window 2.15
import MemoChat 1.0

Window {
    id: root
    width: 280
    height: 420
    minimumWidth: 220
    minimumHeight: 320
    title: "MemoChat Pet"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: false

    property var petController: null

    function openPet() {
        if (petController && petController.sessionId.length === 0) {
            petController.startSession()
        }
        show()
        raise()
    }

    Rectangle {
        anchors.fill: parent
        radius: 18
        color: Qt.rgba(0.13, 0.17, 0.22, 0.22)
        border.color: Qt.rgba(1, 1, 1, 0.28)
    }

    PetScene {
        anchors.fill: parent
        petController: root.petController
        onCloseRequested: root.hide()
    }

    DragHandler {
        target: null
        onActiveChanged: {
            if (active) {
                root.startSystemMove()
            }
        }
    }
}
