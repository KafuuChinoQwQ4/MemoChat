import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root
    property Item backdrop: null
    property string iconSource: "qrc:/res/head_1.jpg"
    signal chooseAvatarRequested()

    implicitWidth: 260
    implicitHeight: 190

    GlassSurface {
        anchors.fill: parent
        backdrop: root.backdrop !== null ? root.backdrop : root
        cornerRadius: 10
        blurRadius: 30
        fillColor: Qt.rgba(1, 1, 1, 0.22)
        strokeColor: Qt.rgba(1, 1, 1, 0.46)
        glowTopColor: Qt.rgba(1, 1, 1, 0.22)
        glowBottomColor: Qt.rgba(1, 1, 1, 0.03)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Label {
                text: "头像"
                color: "#28364a"
                font.pixelSize: 16
                font.bold: true
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 96
                Layout.preferredHeight: 96
                radius: 8
                clip: true
                color: Qt.rgba(0.74, 0.83, 0.93, 0.54)

                Image {
                    anchors.fill: parent
                    source: root.iconSource
                    fillMode: Image.PreserveAspectCrop
                }
            }

            GlassButton {
                Layout.alignment: Qt.AlignHCenter
                text: "更换头像"
                textPixelSize: 13
                cornerRadius: 9
                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.20)
                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.30)
                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.38)
                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                onClicked: root.chooseAvatarRequested()
            }
        }
    }
}
