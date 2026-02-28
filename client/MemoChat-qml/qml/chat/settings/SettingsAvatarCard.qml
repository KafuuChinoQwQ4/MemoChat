import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    property string iconSource: "qrc:/res/head_1.jpg"
    signal chooseAvatarRequested()

    implicitWidth: 260
    implicitHeight: 190

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: "#ffffff"
        border.color: "#dfe5ee"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Label {
                text: "头像"
                color: "#2f3a4a"
                font.pixelSize: 16
                font.bold: true
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 96
                Layout.preferredHeight: 96
                radius: 8
                clip: true
                color: "#dfe5ee"

                Image {
                    anchors.fill: parent
                    source: root.iconSource
                    fillMode: Image.PreserveAspectCrop
                }
            }

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: "更换头像"
                onClicked: root.chooseAvatarRequested()
            }
        }
    }
}
