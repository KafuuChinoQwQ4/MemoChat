import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

Item {
    id: root

    property Item backdrop: null
    property var agentSessions: []
    property var agentGameRooms: []
    property string currentSessionId: ""
    property string currentGameRoomId: ""
    property string currentModel: ""
    property bool busy: false

    property string pendingDeleteSessionId: ""
    property string pendingDeleteRoomId: ""
    property string pendingDeleteTitle: ""
    readonly property bool pendingDeleteIsRoom: pendingDeleteRoomId.length > 0
    readonly property var agentEntryModel: agentEntries(agentSessions, agentGameRooms, currentModel)

    signal newChatRequested()
    signal newSessionRequested()
    signal newGameRequested()
    signal sessionSelected(string sessionId)
    signal sessionDeleted(string sessionId)
    signal gameRoomSelected(string roomId)
    signal gameRoomDeleted(string roomId)

    function roomModeLabel(room) {
        var rulesetId = (room && (room.ruleset_id || room.ruleset)) || ""
        return rulesetId === "multi_ai_chat.test" ? "多 AI 聊天" : "Game 模式"
    }

    function agentEntries(sessions, rooms, currentModelName) {
        var rows = []
        for (var i = 0; i < (sessions ? sessions.length : 0); ++i) {
            var session = sessions[i] || {}
            rows.push({
                "kind": "session",
                "entry_id": session.session_id || "",
                "title": session.title || "新会话",
                "subtitle": session.model_name || currentModelName || "AI 会话",
                "status": i,
                "session_id": session.session_id || "",
                "room_id": "",
                "model_name": session.model_name || ""
            })
        }
        for (var j = 0; j < (rooms ? rooms.length : 0); ++j) {
            var room = rooms[j] || {}
            rows.push({
                "kind": "room",
                "entry_id": room.room_id || room.id || "",
                "title": room.title || room.name || "对话房间",
                "subtitle": (room.status || "ready") + " · " + root.roomModeLabel(room),
                "status": (sessions ? sessions.length : 0) + j,
                "session_id": "",
                "room_id": room.room_id || room.id || "",
                "model_name": ""
            })
        }
        return rows
    }

    function openDeleteDialog(entry) {
        if (!entry) {
            return
        }
        if (entry.kind === "room") {
            root.pendingDeleteRoomId = entry.room_id || entry.entry_id || ""
            root.pendingDeleteSessionId = ""
        } else {
            root.pendingDeleteSessionId = entry.session_id || entry.entry_id || ""
            root.pendingDeleteRoomId = ""
        }
        root.pendingDeleteTitle = entry.title || (entry.kind === "room" ? "对话房间" : "新会话")
        if (root.pendingDeleteSessionId.length > 0 || root.pendingDeleteRoomId.length > 0) {
            agentDeleteDialog.open()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "房间"
                color: "#2a3649"
                font.pixelSize: 15
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            GlassButton {
                id: agentNewButton
                Layout.preferredWidth: 64
                Layout.preferredHeight: 30
                text: "新建"
                textPixelSize: 12
                cornerRadius: 8
                normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.22)
                hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.32)
                pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.40)
                onClicked: agentNewMenuPopup.open()
            }

            Popup {
                id: agentNewMenuPopup
                width: 180
                height: agentNewMenuColumn.implicitHeight + 16
                x: Math.max(0, agentNewButton.x)
                y: agentNewButton.y + agentNewButton.height + 6
                padding: 0
                modal: false
                focus: true
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                background: Rectangle {
                    color: "transparent"
                }

                contentItem: GlassSurface {
                    backdrop: root.backdrop !== null ? root.backdrop : root
                    cornerRadius: 12
                    blurRadius: 18
                    fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.90)
                    strokeColor: Qt.rgba(1, 1, 1, 0.58)
                    glowTopColor: Qt.rgba(1, 1, 1, 0.28)
                    glowBottomColor: Qt.rgba(1, 1, 1, 0.06)

                    ColumnLayout {
                        id: agentNewMenuColumn
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        GlassButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            text: "单 AI 私聊"
                            textPixelSize: 13
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.18)
                            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.28)
                            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.36)
                            onClicked: {
                                agentNewMenuPopup.close()
                                root.newChatRequested()
                            }
                        }

                        GlassButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            text: "多 AI 聊天"
                            textPixelSize: 13
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.30, 0.58, 0.36, 0.18)
                            hoverColor: Qt.rgba(0.30, 0.58, 0.36, 0.28)
                            pressedColor: Qt.rgba(0.30, 0.58, 0.36, 0.36)
                            onClicked: {
                                agentNewMenuPopup.close()
                                root.newSessionRequested()
                            }
                        }

                        GlassButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            text: "Game 模式"
                            textPixelSize: 13
                            cornerRadius: 8
                            normalColor: Qt.rgba(0.48, 0.42, 0.72, 0.18)
                            hoverColor: Qt.rgba(0.48, 0.42, 0.72, 0.28)
                            pressedColor: Qt.rgba(0.48, 0.42, 0.72, 0.36)
                            onClicked: {
                                agentNewMenuPopup.close()
                                root.newGameRequested()
                            }
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.agentEntryModel.length > 0 ? ("共 " + root.agentEntryModel.length + " 项") : "创建会话或房间后会显示在这里"
            color: "#6a7b92"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        ListView {
            id: agentSessionList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.agentEntryModel
            spacing: 6
            ScrollBar.vertical: GlassScrollBar { }

            delegate: Rectangle {
                width: ListView.view.width
                implicitHeight: 62
                radius: 10
                color: {
                    if (modelData.kind === "room" && (modelData.room_id || "") === root.currentGameRoomId) {
                        return Qt.rgba(0.54, 0.70, 0.93, 0.22)
                    }
                    if (modelData.kind === "session" && (modelData.session_id || "") === root.currentSessionId) {
                        return Qt.rgba(0.54, 0.70, 0.93, 0.22)
                    }
                    return sessionHover.containsMouse ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.04)
                }
                border.color: (modelData.kind === "room" && (modelData.room_id || "") === root.currentGameRoomId)
                              || (modelData.kind === "session" && (modelData.session_id || "") === root.currentSessionId)
                              ? Qt.rgba(0.54, 0.70, 0.93, 0.54)
                              : Qt.rgba(1, 1, 1, 0.24)

                MouseArea {
                    id: sessionHover
                    z: 0
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (modelData.kind === "room") {
                            const roomId = modelData.room_id || modelData.id || ""
                            if (roomId.length > 0) {
                                root.gameRoomSelected(roomId)
                            }
                        } else {
                            const sessionId = modelData.session_id || modelData.entry_id || ""
                            if (sessionId.length > 0) {
                                root.sessionSelected(sessionId)
                            }
                        }
                    }
                }

                RowLayout {
                    z: 1
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 10
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Label {
                            Layout.fillWidth: true
                            text: modelData.kind === "room" ? "对话房间" : "会话"
                            color: "#6a7b92"
                            font.pixelSize: 10
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: modelData.title || modelData.name || "未命名"
                            color: "#273449"
                            font.pixelSize: 13
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: modelData.subtitle || ""
                            color: "#6a7b92"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        radius: 12
                        color: sessionDeleteArea.pressed ? Qt.rgba(0.89, 0.27, 0.27, 0.30)
                              : sessionDeleteArea.containsMouse ? Qt.rgba(0.89, 0.27, 0.27, 0.24)
                                                                : Qt.rgba(0.89, 0.27, 0.27, 0.16)
                        visible: modelData.kind === "session" || modelData.kind === "room"
                        opacity: root.busy ? 0.45 : 1.0

                        Label {
                            anchors.centerIn: parent
                            text: "×"
                            color: "#b84b4b"
                            font.pixelSize: 13
                            font.bold: true
                        }

                        MouseArea {
                            id: sessionDeleteArea
                            anchors.fill: parent
                            hoverEnabled: true
                            enabled: (modelData.kind === "session" || modelData.kind === "room") && !root.busy
                            cursorShape: Qt.PointingHandCursor
                            ToolTip.visible: containsMouse
                            ToolTip.delay: 120
                            ToolTip.text: modelData.kind === "room" ? "清除对话房间" : "删除会话"
                            onClicked: function(mouse) {
                                mouse.accepted = true
                                root.openDeleteDialog(modelData)
                            }
                        }
                    }
                }
            }
        }
    }

    GlassSurface {
        anchors.centerIn: parent
        width: 210
        height: 82
        visible: agentSessionList.count === 0
        backdrop: root.backdrop !== null ? root.backdrop : root
        cornerRadius: 10
        blurRadius: 16
        fillColor: Qt.rgba(1, 1, 1, 0.18)
        strokeColor: Qt.rgba(1, 1, 1, 0.38)

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 4

            Label {
                text: "暂无会话或房间"
                color: "#2a3649"
                font.pixelSize: 13
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                text: "点击右上角「新建」创建会话或房间"
                color: "#6a7b92"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Popup {
        id: agentDeleteDialog
        modal: true
        focus: true
        width: Math.min(320, parent.width - 32)
        height: 184
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Overlay.modal: Rectangle {
            color: Qt.rgba(20, 28, 40, 0.26)
        }

        background: Rectangle {
            radius: 16
            color: "transparent"
        }

        onClosed: {
            root.pendingDeleteSessionId = ""
            root.pendingDeleteRoomId = ""
            root.pendingDeleteTitle = ""
        }

        contentItem: GlassSurface {
            backdrop: root.backdrop !== null ? root.backdrop : root
            cornerRadius: 16
            blurRadius: 20
            fillColor: Qt.rgba(0.98, 0.99, 1.0, 0.88)
            strokeColor: Qt.rgba(1, 1, 1, 0.58)
            glowTopColor: Qt.rgba(1, 1, 1, 0.30)
            glowBottomColor: Qt.rgba(1, 1, 1, 0.06)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                Label {
                    Layout.fillWidth: true
                    text: root.pendingDeleteIsRoom ? "清除对话房间" : "删除 AI 会话"
                    color: "#263448"
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: root.pendingDeleteIsRoom
                          ? ("确认清除「" + root.pendingDeleteTitle + "」？")
                          : ("确认删除「" + root.pendingDeleteTitle + "」？")
                    color: "#5f6f85"
                    font.pixelSize: 14
                    wrapMode: Text.Wrap
                    verticalAlignment: Text.AlignVCenter
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: "取消"
                        textPixelSize: 13
                        cornerRadius: 10
                        normalColor: Qt.rgba(0.54, 0.60, 0.68, 0.18)
                        hoverColor: Qt.rgba(0.54, 0.60, 0.68, 0.26)
                        pressedColor: Qt.rgba(0.54, 0.60, 0.68, 0.34)
                        onClicked: agentDeleteDialog.close()
                    }

                    GlassButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: root.pendingDeleteIsRoom ? "清除" : "删除"
                        textPixelSize: 13
                        textColor: "#b83f4a"
                        cornerRadius: 10
                        normalColor: Qt.rgba(0.89, 0.27, 0.27, 0.16)
                        hoverColor: Qt.rgba(0.89, 0.27, 0.27, 0.24)
                        pressedColor: Qt.rgba(0.89, 0.27, 0.27, 0.32)
                        onClicked: {
                            const roomId = root.pendingDeleteRoomId
                            const sessionId = root.pendingDeleteSessionId
                            if (roomId.length > 0) {
                                root.gameRoomDeleted(roomId)
                            } else if (sessionId.length > 0) {
                                root.sessionDeleted(sessionId)
                            }
                            agentDeleteDialog.close()
                        }
                    }
                }
            }
        }
    }
}
