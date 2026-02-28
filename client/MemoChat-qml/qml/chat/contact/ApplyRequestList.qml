import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property var applyModel
    signal approveClicked(int uid, string name)

    ListView {
        id: applyList
        anchors.fill: parent
        clip: true
        spacing: 0
        model: root.applyModel

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

    Label {
        anchors.centerIn: parent
        text: "暂无新的好友申请"
        visible: applyList.count === 0
        color: "#8e9aac"
        font.pixelSize: 14
    }
}
