import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../../components"

Item {
    id: root

    property Item backdrop: null
    property var applyModel
    signal approveClicked(int uid, string name)

    ListView {
        id: applyList
        anchors.fill: parent
        anchors.margins: 8
        clip: true
        spacing: 8
        model: root.applyModel
        ScrollBar.vertical: GlassScrollBar { }

        delegate: ApplyRequestDelegate {
            width: ListView.view.width
            uid: model.uid
            name: model.name
            desc: model.desc
            icon: model.icon
            approved: model.approved
            pending: model.pending
            onApproveClicked: root.approveClicked(uid, name)
        }
    }

    GlassSurface {
        anchors.centerIn: parent
        width: 220
        height: 94
        visible: applyList.count === 0
        backdrop: root.backdrop !== null ? root.backdrop : root
        cornerRadius: 10
        blurRadius: 28
        fillColor: Qt.rgba(1, 1, 1, 0.20)
        strokeColor: Qt.rgba(1, 1, 1, 0.42)

        Label {
            anchors.centerIn: parent
            text: "暂无新的好友申请"
            color: "#6a7b92"
            font.pixelSize: 14
        }
    }
}
