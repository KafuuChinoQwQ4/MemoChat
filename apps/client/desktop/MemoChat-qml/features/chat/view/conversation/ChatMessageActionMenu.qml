pragma ComponentBehavior: Bound

import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: root

    readonly property int menuContentWidth: 136
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
    property bool showRevokeExpiredHint: false

    signal replyRequested(string msgId, string senderName, string previewText)
    signal mentionRequested(string mentionText)
    signal forwardRequested(string msgId)
    signal editRequested(string msgId, string text)
    signal revokeRequested(string msgId)
    signal translateRequested(string msgId, string text)

    function actionRows() {
        const rows = []
        if (root.canReply) {
            rows.push({ "type": "action", "key": "reply", "text": "回复", "enabled": true })
        }
        if (root.canMention) {
            rows.push({ "type": "action", "key": "mention", "text": "@Ta", "enabled": true })
        }
        if (root.canForward) {
            rows.push({ "type": "action", "key": "forward", "text": "转发", "enabled": true })
        }
        if (root.canEdit) {
            rows.push({ "type": "action", "key": "edit", "text": "编辑", "enabled": true })
        }
        if (root.msgType === "text" && root.content.length > 0) {
            rows.push({ "type": "action", "key": "translate", "text": "翻译", "enabled": true })
        }
        if (root.canRevoke || root.showRevokeExpiredHint) {
            rows.push({ "type": "separator" })
        }
        if (root.canRevoke) {
            rows.push({ "type": "action", "key": "revoke", "text": "撤回", "enabled": true })
        } else if (root.showRevokeExpiredHint) {
            rows.push({ "type": "action", "key": "revokeExpired", "text": "撤回 (已超过5分钟)", "enabled": false })
        }
        return rows
    }

    function triggerRow(rowKey) {
        if (rowKey === "reply") {
            root.replyRequested(root.msgId, root.senderName, root.previewText)
        } else if (rowKey === "mention") {
            root.mentionRequested("@" + root.senderName + " ")
        } else if (rowKey === "forward") {
            root.forwardRequested(root.msgId)
        } else if (rowKey === "edit") {
            root.editRequested(root.msgId, root.content)
        } else if (rowKey === "revoke") {
            root.revokeRequested(root.msgId)
        } else if (rowKey === "translate") {
            root.translateRequested(root.msgId, root.content)
        }
        root.close()
    }

    function clampedPoint(pointX, pointY, menuWidth, menuHeight, boundaryWidth, boundaryHeight) {
        const maxX = Math.max(0, boundaryWidth - menuWidth)
        const maxY = Math.max(0, boundaryHeight - menuHeight)
        return Qt.point(Math.max(0, Math.min(maxX, pointX)),
                        Math.max(0, Math.min(maxY, pointY)))
    }

    function bestPoint(pointX, pointY, menuWidth, menuHeight, boundaryWidth, boundaryHeight) {
        const margin = 4
        const candidates = [
            Qt.point(pointX + margin, pointY + margin),
            Qt.point(pointX + margin, pointY - menuHeight - margin),
            Qt.point(pointX - menuWidth - margin, pointY + margin),
            Qt.point(pointX - menuWidth - margin, pointY - menuHeight - margin)
        ]
        for (let i = 0; i < candidates.length; ++i) {
            const candidate = candidates[i]
            if (candidate.x >= 0
                    && candidate.y >= 0
                    && candidate.x + menuWidth <= boundaryWidth
                    && candidate.y + menuHeight <= boundaryHeight) {
                return candidate
            }
        }
        return root.clampedPoint(pointX + margin,
                                 pointY + margin,
                                 menuWidth,
                                 menuHeight,
                                 boundaryWidth,
                                 boundaryHeight)
    }

    function openAt(pointX, pointY, boundaryWidth, boundaryHeight) {
        if (root.actionRows().length === 0) {
            return
        }
        const menuWidth = root.implicitWidth > 0 ? root.implicitWidth : 150
        const menuHeight = root.implicitHeight > 0 ? root.implicitHeight : 48
        const position = root.bestPoint(pointX, pointY, menuWidth, menuHeight, boundaryWidth, boundaryHeight)
        root.x = position.x
        root.y = position.y
        root.open()
    }

    transformOrigin: Item.TopLeft
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    implicitWidth: 150
    implicitHeight: menuColumn.implicitHeight + topPadding + bottomPadding
    padding: 7

    background: Rectangle {
        radius: 10
        color: "#f8fbff"
        border.width: 1
        border.color: Qt.rgba(0.60, 0.68, 0.78, 0.72)
    }

    contentItem: Column {
        id: menuColumn
        width: root.menuContentWidth
        spacing: 0

        Repeater {
            model: root.actionRows()

            delegate: Item {
                id: rowRoot
                width: root.menuContentWidth
                height: implicitHeight
                implicitHeight: rowRoot.rowData.type === "separator" ? 9 : 34

                required property var modelData
                readonly property var rowData: modelData
                readonly property bool rowEnabled: rowData.enabled !== false

                Rectangle {
                    anchors.fill: parent
                    radius: 7
                    visible: rowRoot.rowData.type !== "separator"
                    color: !rowRoot.rowEnabled ? "transparent"
                           : rowMouse.pressed ? Qt.rgba(0.35, 0.61, 0.90, 0.20)
                           : rowMouse.containsMouse ? Qt.rgba(0.35, 0.61, 0.90, 0.12)
                                                    : "transparent"

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        text: rowRoot.rowData.text || ""
                        color: text.indexOf("撤回") === 0
                               ? (rowRoot.rowEnabled ? "#c94444" : "#9aa5b3")
                               : (rowRoot.rowEnabled ? "#253247" : "#9aa5b3")
                        font.pixelSize: 13
                        font.bold: rowMouse.containsMouse && rowRoot.rowEnabled
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: rowRoot.rowEnabled
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: root.triggerRow(rowRoot.rowData.key)
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    height: 1
                    visible: rowRoot.rowData.type === "separator"
                    color: Qt.rgba(0.60, 0.68, 0.78, 0.26)
                }
            }
        }
    }
}
