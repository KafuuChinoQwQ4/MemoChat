import QtQuick 2.15
import QtQuick.Controls 2.15

Menu {
    id: root

    property string msgId: ""
    property string msgType: "text"
    property string content: ""
    property string senderName: ""
    property string previewText: ""
    property bool canReply: false
    property bool canMention: false
    property bool canForward: false
    property bool canEdit: false
    property bool canRevoke: false

    signal replyRequested(string msgId, string senderName, string previewText)
    signal mentionRequested(string mentionText)
    signal forwardRequested(string msgId)
    signal editRequested(string msgId, string text)
    signal revokeRequested(string msgId)
    signal translateRequested(string msgId, string text)

    function openAt(pointX, pointY, boundaryWidth, boundaryHeight) {
        root.x = Math.max(0, Math.min(boundaryWidth - root.width, pointX))
        root.y = Math.max(0, Math.min(boundaryHeight - root.height, pointY))
        root.open()
    }

    transformOrigin: Item.TopLeft

    MenuItem {
        visible: root.canReply
        text: "回复"
        onTriggered: root.replyRequested(root.msgId, root.senderName, root.previewText)
    }
    MenuItem {
        visible: root.canMention
        text: "@Ta"
        onTriggered: root.mentionRequested("@" + root.senderName + " ")
    }
    MenuItem {
        visible: root.canForward
        text: "转发"
        onTriggered: root.forwardRequested(root.msgId)
    }
    MenuItem {
        visible: root.canEdit
        text: "编辑"
        onTriggered: root.editRequested(root.msgId, root.content)
    }
    MenuItem {
        visible: root.msgType === "text" && root.content.length > 0
        text: "翻译"
        onTriggered: root.translateRequested(root.msgId, root.content)
    }
    MenuItem {
        visible: root.canRevoke
        text: "撤回"
        onTriggered: root.revokeRequested(root.msgId)
    }
}
