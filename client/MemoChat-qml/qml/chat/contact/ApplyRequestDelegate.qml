import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Rectangle {
    id: root
    color: Qt.rgba(1, 1, 1, 0.26)
    border.color: Qt.rgba(1, 1, 1, 0.44)
    border.width: 1
    implicitHeight: 82
    radius: 10

    property int uid: 0
    property string name: ""
    property string desc: ""
    property string icon: ""
    property bool approved: false
    property bool pending: false

    signal approveClicked(int uid, string name)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 45
            Layout.preferredHeight: 45
            radius: 4
            clip: true
            color: Qt.rgba(0.74, 0.83, 0.93, 0.52)

            Image {
                anchors.fill: parent
                source: root.icon
                fillMode: Image.PreserveAspectCrop
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: root.name
                color: "#28364b"
                font.pixelSize: 16
            }

            Label {
                text: root.desc.length > 0 ? root.desc : ("UID: " + root.uid)
                color: "#63748c"
                elide: Text.ElideRight
                Layout.fillWidth: true
                font.pixelSize: 14
            }
        }

        GlassButton {
            Layout.preferredWidth: 84
            Layout.preferredHeight: 32
            cornerRadius: 8
            text: root.approved ? "已添加" : (root.pending ? "认证中" : "添加")
            enabled: !root.approved && !root.pending
            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.22)
            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.40)
            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
            onClicked: root.approveClicked(root.uid, root.name)
        }
    }
}
