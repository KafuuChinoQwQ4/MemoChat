import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#f1f2f3"
    border.color: "#dbd9d9"
    border.width: 1
    implicitHeight: 82

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
            color: "#dfe4eb"

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
                color: "#000000"
                font.pixelSize: 16
            }

            Label {
                text: root.desc.length > 0 ? root.desc : ("UID: " + root.uid)
                color: "#a2a2a2"
                elide: Text.ElideRight
                Layout.fillWidth: true
                font.pixelSize: 14
            }
        }

        Button {
            Layout.preferredWidth: 84
            Layout.preferredHeight: 30
            text: root.approved ? "已添加" : (root.pending ? "认证中" : "添加")
            enabled: !root.approved && !root.pending
            onClicked: root.approveClicked(root.uid, root.name)
        }
    }
}
