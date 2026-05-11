import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import MemoChat 1.0

Item {
    id: root
    property var petController: null
    signal closeRequested()

    Live2DRenderItem {
        id: live2d
        anchors.fill: parent
        anchors.margins: 8
        expression: root.petController ? root.petController.expression : "neutral"
        motion: root.petController ? root.petController.motion : "idle"
        emotion: root.petController ? root.petController.emotion : "neutral"
        intensity: root.petController ? root.petController.intensity : 0.35
        gazeX: root.petController ? root.petController.gazeX : 0.5
        gazeY: root.petController ? root.petController.gazeY : 0.5
        lipSyncValue: root.petController ? root.petController.lipSyncValue : 0
    }

    Connections {
        target: root.petController
        function onControlEventReceived(event) {
            live2d.applyControlEvent(event)
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: composer.top
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        anchors.bottomMargin: 8
        height: Math.min(58, speechLabel.implicitHeight + 18)
        radius: 8
        color: Qt.rgba(1, 1, 1, 0.68)
        border.color: Qt.rgba(1, 1, 1, 0.75)
        visible: root.petController && root.petController.speechText.length > 0

        Label {
            id: speechLabel
            anchors.fill: parent
            anchors.margins: 9
            text: root.petController ? root.petController.speechText : ""
            color: "#253142"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }
    }

    RowLayout {
        id: topControls
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        height: 28
        spacing: 6

        Rectangle {
            Layout.preferredWidth: 9
            Layout.preferredHeight: 9
            radius: 5
            color: root.petController && root.petController.streaming ? "#42b883" : "#d4a23f"
        }

        Label {
            Layout.fillWidth: true
            text: root.petController && root.petController.statusText.length > 0
                  ? root.petController.statusText
                  : "桌宠"
            color: "#f7fbff"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        ToolButton {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            text: "x"
            onClicked: root.closeRequested()
        }
    }

    RowLayout {
        id: composer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.bottomMargin: 14
        height: 34
        spacing: 8

        TextField {
            id: input
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            placeholderText: "对桌宠说点什么"
            font.pixelSize: 12
            selectByMouse: true
            background: Rectangle {
                radius: 8
                color: Qt.rgba(255, 255, 255, 0.74)
                border.color: input.activeFocus ? "#74b2ba" : Qt.rgba(255, 255, 255, 0.76)
            }
            onAccepted: sendButton.clicked()
        }

        Button {
            id: sendButton
            Layout.preferredWidth: 54
            Layout.preferredHeight: 34
            text: "发送"
            enabled: root.petController && input.text.trim().length > 0
            onClicked: {
                if (!root.petController) {
                    return
                }
                root.petController.sendText(input.text)
                input.text = ""
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topControls.bottom
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.topMargin: 2
        height: errorLabel.implicitHeight + 12
        radius: 8
        color: Qt.rgba(0.88, 0.22, 0.22, 0.72)
        visible: root.petController && root.petController.error.length > 0

        Label {
            id: errorLabel
            anchors.fill: parent
            anchors.margins: 6
            text: root.petController ? root.petController.error : ""
            color: "white"
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }
}
