import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"
import "contact"

Rectangle {
    id: root
    color: "transparent"
    width: 250
    property Item backdrop: null

    property int currentTab: 0
    property int currentDialogUid: 0
    property var dialogModel
    property var chatModel
    property var groupModel
    property var contactModel
    property var searchModel
    property bool searchPending: false
    property string searchStatusText: ""
    property bool searchStatusError: false
    property bool canLoadMoreChats: false
    property bool chatLoadingMore: false
    property bool hasPendingApply: false
    property bool canLoadMoreContacts: false
    property bool contactLoadingMore: false
    property string groupStatusText: ""
    property bool groupStatusError: false
    property int sessionFilter: 0 // 0 all, 1 private, 2 group
    property int selectedDialogUid: 0
    property int selectedChatUid: 0
    property int selectedGroupUid: 0
    property real _sessionLastContentY: 0
    property bool _chatLoadBottomArmed: true
    property real _chatLoadThresholdPx: 48
    property real _chatLoadRearmOffsetPx: 96

    signal dialogUidSelected(int uid)
    signal chatIndexSelected(int index)
    signal groupIndexSelected(int index)
    signal requestChatLoadMore()
    signal searchRequested(string uidText)
    signal searchCleared()
    signal addFriendRequested(int uid, string bakName, var tags)
    signal openApplyRequested()
    signal contactIndexSelected(int index)
    signal requestContactLoadMore()
    signal createGroupRequested()
    signal dialogPinToggled(int uid)
    signal dialogMuteToggled(int uid)
    signal dialogMarkRead(int uid)
    signal dialogDraftCleared(int uid)

    onCurrentTabChanged: if (currentTab !== 0) { sessionFilter = 0 }
    onSessionFilterChanged: {
        _sessionLastContentY = sessionList.contentY
        _chatLoadBottomArmed = true
        syncCurrentSelection()
    }

    onCurrentDialogUidChanged: syncCurrentSelection()

    function activateSession(uidValue, indexValue) {
        sessionList.currentIndex = indexValue
        if (root.sessionFilter === 2) {
            root.selectedGroupUid = uidValue
            root.groupIndexSelected(indexValue)
        } else if (root.sessionFilter === 1) {
            root.selectedChatUid = uidValue
            root.chatIndexSelected(indexValue)
        } else {
            root.selectedDialogUid = uidValue
            root.dialogUidSelected(uidValue)
        }
    }

    function syncCurrentSelection() {
        const modelRef = root.sessionFilter === 0 ? root.dialogModel
                        : (root.sessionFilter === 1 ? root.chatModel : root.groupModel)
        if (!modelRef || sessionList.count <= 0) {
            sessionList.currentIndex = -1
            return
        }

        let targetUid = 0
        if (root.sessionFilter === 0) {
            targetUid = root.currentDialogUid
        } else if (root.sessionFilter === 1) {
            targetUid = root.currentDialogUid > 0 ? root.currentDialogUid : 0
        } else {
            targetUid = root.currentDialogUid < 0 ? root.currentDialogUid : 0
        }

        if (targetUid === 0) {
            if (root.sessionFilter === 0 && sessionList.currentIndex < 0 && modelRef.get) {
                const firstItem = modelRef.get(0)
                const firstUid = firstItem && firstItem.uid !== undefined ? firstItem.uid : 0
                if (firstUid !== 0) {
                    root.selectedDialogUid = firstUid
                    sessionList.currentIndex = 0
                    root.dialogUidSelected(firstUid)
                    return
                }
            }
            sessionList.currentIndex = -1
            return
        }

        const targetIndex = modelRef.indexOfUid ? modelRef.indexOfUid(targetUid) : -1
        sessionList.currentIndex = targetIndex
        if (targetIndex < 0) {
            return
        }

        if (root.sessionFilter === 2) {
            root.selectedGroupUid = targetUid
        } else if (root.sessionFilter === 1) {
            root.selectedChatUid = targetUid
        } else {
            root.selectedDialogUid = targetUid
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: Qt.rgba(1, 1, 1, 0.10)
            border.color: Qt.rgba(1, 1, 1, 0.30)

            RowLayout {
                anchors.fill: parent
                anchors.margins: 9
                spacing: 6

                GlassTextField {
                    id: searchField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    backdrop: root.backdrop !== null ? root.backdrop : root
                    blurRadius: 18
                    cornerRadius: 9
                    leftInset: 12
                    rightInset: 12
                    textPixelSize: 14
                    textHorizontalAlignment: TextInput.AlignLeft
                    placeholderText: "搜索用户ID（u#########）"
                    onTextChanged: root.searchCleared()
                    onAccepted: root.searchRequested(searchField.text)
                }

                GlassButton {
                    Layout.preferredWidth: 54
                    Layout.preferredHeight: 34
                    text: "查找"
                    textPixelSize: 13
                    cornerRadius: 9
                    normalColor: Qt.rgba(0.55, 0.70, 0.92, 0.24)
                    hoverColor: Qt.rgba(0.55, 0.70, 0.92, 0.34)
                    pressedColor: Qt.rgba(0.55, 0.70, 0.92, 0.42)
                    disabledColor: Qt.rgba(0.52, 0.56, 0.62, 0.18)
                    onClicked: root.searchRequested(searchField.text)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Qt.rgba(1, 1, 1, 0.08)
            border.color: Qt.rgba(1, 1, 1, 0.28)

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length > 0
                    color: "transparent"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label {
                                text: searchPending ? "搜索中..." : "搜索用户ID"
                                color: "#415066"
                                Layout.fillWidth: true
                            }
                            GlassButton {
                                text: "查找"
                                enabled: !searchPending
                                implicitWidth: 62
                                implicitHeight: 30
                                cornerRadius: 8
                                normalColor: Qt.rgba(0.54, 0.70, 0.93, 0.22)
                                hoverColor: Qt.rgba(0.54, 0.70, 0.93, 0.32)
                                pressedColor: Qt.rgba(0.54, 0.70, 0.93, 0.40)
                                disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                onClicked: root.searchRequested(searchField.text)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.searchStatusText
                            visible: text.length > 0
                            color: root.searchStatusError ? "#cc4a4a" : "#2a7f62"
                            wrapMode: Text.Wrap
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ListView {
                                id: searchResultList
                                anchors.fill: parent
                                clip: true
                                model: root.searchModel
                                ScrollBar.vertical: GlassScrollBar { }

                                delegate: Rectangle {
                                    width: ListView.view.width
                                    height: 86
                                    radius: 8
                                    color: Qt.rgba(1, 1, 1, 0.30)
                                    border.color: Qt.rgba(1, 1, 1, 0.48)

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 10
                                        spacing: 10

                                        Rectangle {
                                            Layout.preferredWidth: 50
                                            Layout.preferredHeight: 50
                                            radius: 6
                                            clip: true
                                            color: Qt.rgba(0.74, 0.82, 0.92, 0.50)
                                            Image {
                                                anchors.fill: parent
                                                fillMode: Image.PreserveAspectCrop
                                                property string fallbackSource: "qrc:/res/head_1.jpg"
                                                property string baseSource: (icon && icon.length > 0) ? icon : fallbackSource
                                                property bool loadFailed: false
                                                source: loadFailed ? fallbackSource : baseSource
                                                onBaseSourceChanged: loadFailed = false
                                                onStatusChanged: {
                                                    if (status === Image.Error) {
                                                        loadFailed = true
                                                    }
                                                }
                                            }
                                        }

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2
                                            Label { text: name; font.bold: true; color: "#273449" }
                                            Label { text: "ID: " + (userId && userId.length > 0 ? userId : uid); color: "#5f6f85" }
                                            Label {
                                                text: desc
                                                color: "#5f6f85"
                                                elide: Text.ElideRight
                                                Layout.fillWidth: true
                                            }
                                        }

                                        GlassButton {
                                            text: "加好友"
                                            implicitWidth: 70
                                            implicitHeight: 30
                                            cornerRadius: 8
                                            normalColor: Qt.rgba(0.35, 0.61, 0.90, 0.24)
                                            hoverColor: Qt.rgba(0.35, 0.61, 0.90, 0.34)
                                            pressedColor: Qt.rgba(0.35, 0.61, 0.90, 0.42)
                                            disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                            onClicked: addFriendDialog.openFor(uid, name, desc, icon)
                                        }
                                    }
                                }
                            }

                            GlassSurface {
                                anchors.centerIn: parent
                                width: 190
                                height: 64
                                visible: searchField.text.length > 0
                                         && !searchPending
                                         && searchResultList.count === 0
                                backdrop: root.backdrop !== null ? root.backdrop : root
                                cornerRadius: 10
                                blurRadius: 16
                                fillColor: Qt.rgba(1, 1, 1, 0.20)
                                strokeColor: Qt.rgba(1, 1, 1, 0.42)

                                Label {
                                    anchors.centerIn: parent
                                    text: "未找到匹配用户"
                                    color: "#6a7b92"
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length === 0 && root.currentTab === 0

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 76
                            color: Qt.rgba(1, 1, 1, 0.08)
                            border.color: Qt.rgba(1, 1, 1, 0.22)

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    GlassButton {
                                        Layout.fillWidth: true
                                        implicitHeight: 30
                                        text: "全部"
                                        cornerRadius: 8
                                        normalColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20)
                                        hoverColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28)
                                        pressedColor: root.sessionFilter === 0 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34)
                                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                        onClicked: root.sessionFilter = 0
                                    }

                                    GlassButton {
                                        Layout.fillWidth: true
                                        implicitHeight: 30
                                        id: privateFilterBtn
                                        text: ""
                                        cornerRadius: 8
                                        normalColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20)
                                        hoverColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28)
                                        pressedColor: root.sessionFilter === 1 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34)
                                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                        onClicked: root.sessionFilter = 1

                                        Image {
                                            anchors.centerIn: parent
                                            width: 16
                                            height: 16
                                            source: "qrc:/icons/user.png"
                                            fillMode: Image.PreserveAspectFit
                                        }

                                        ToolTip.visible: privateFilterBtn.hovering
                                        ToolTip.delay: 120
                                        ToolTip.text: "私聊"
                                    }

                                    GlassButton {
                                        Layout.fillWidth: true
                                        implicitHeight: 30
                                        id: groupFilterBtn
                                        text: ""
                                        cornerRadius: 8
                                        normalColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.28) : Qt.rgba(0.48, 0.56, 0.66, 0.20)
                                        hoverColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.38) : Qt.rgba(0.48, 0.56, 0.66, 0.28)
                                        pressedColor: root.sessionFilter === 2 ? Qt.rgba(0.36, 0.62, 0.92, 0.45) : Qt.rgba(0.48, 0.56, 0.66, 0.34)
                                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                        onClicked: root.sessionFilter = 2

                                        Image {
                                            anchors.centerIn: parent
                                            width: 16
                                            height: 16
                                            source: "qrc:/icons/team.png"
                                            fillMode: Image.PreserveAspectFit
                                        }

                                        ToolTip.visible: groupFilterBtn.hovering
                                        ToolTip.delay: 120
                                        ToolTip.text: "群组"
                                    }

                                    GlassButton {
                                        implicitWidth: 42
                                        implicitHeight: 30
                                        text: "+"
                                        visible: root.sessionFilter === 2
                                        cornerRadius: 8
                                        normalColor: Qt.rgba(0.28, 0.70, 0.58, 0.24)
                                        hoverColor: Qt.rgba(0.28, 0.70, 0.58, 0.34)
                                        pressedColor: Qt.rgba(0.28, 0.70, 0.58, 0.42)
                                        disabledColor: Qt.rgba(0.52, 0.57, 0.64, 0.16)
                                        onClicked: root.createGroupRequested()
                                    }

                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.groupStatusText
                                    visible: root.sessionFilter === 2 && text.length > 0
                                    color: root.groupStatusError ? "#cc4a4a" : "#2a7f62"
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        ListView {
                            id: sessionList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            interactive: false
                            model: root.sessionFilter === 0 ? root.dialogModel
                                  : (root.sessionFilter === 1 ? root.chatModel : root.groupModel)
                            ScrollBar.vertical: GlassScrollBar { }
                            WheelHandler {
                                target: null
                                onWheel: function(event) {
                                    const maxY = Math.max(0, sessionList.contentHeight - sessionList.height)
                                    let deltaY = 0
                                    if (event.pixelDelta.y !== 0) {
                                        deltaY = event.pixelDelta.y
                                    } else if (event.angleDelta.y !== 0) {
                                        deltaY = (event.angleDelta.y / 120) * 48
                                    }
                                    if (deltaY === 0) {
                                        return
                                    }
                                    const nextY = Math.max(0, Math.min(maxY, sessionList.contentY - deltaY))
                                    if (Math.abs(nextY - sessionList.contentY) > 0.1) {
                                        sessionList.contentY = nextY
                                        event.accepted = true
                                    }
                                }
                            }
                            onContentYChanged: {
                                const movingDown = contentY > root._sessionLastContentY + 0.5
                                const nearBottom = (contentY + height) >= (contentHeight - root._chatLoadThresholdPx)
                                root._sessionLastContentY = contentY

                                if ((contentY + height) < (contentHeight - root._chatLoadRearmOffsetPx)) {
                                    root._chatLoadBottomArmed = true
                                }

                                if (root.sessionFilter !== 1) {
                                    return
                                }
                                if (!root.canLoadMoreChats || root.chatLoadingMore) {
                                    return
                                }
                                if (contentHeight <= height + 1) {
                                    return
                                }
                                if (movingDown && nearBottom && root._chatLoadBottomArmed) {
                                    root._chatLoadBottomArmed = false
                                    root.requestChatLoadMore()
                                }
                            }
                            onCountChanged: {
                                root.syncCurrentSelection()
                            }

                            footer: Item {
                                width: sessionList.width
                                height: (root.sessionFilter === 1 && (root.chatLoadingMore || root.canLoadMoreChats)) ? 40 : 0

                                Label {
                                    anchors.centerIn: parent
                                    text: root.chatLoadingMore ? "加载中..." : (root.canLoadMoreChats ? "上滑加载更多" : "")
                                    color: "#8e9aac"
                                    font.pixelSize: 12
                                }
                            }

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 64
                                color: ListView.isCurrentItem ? Qt.rgba(0.77, 0.86, 0.97, 0.32) : "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 8

                                    Rectangle {
                                        Layout.preferredWidth: 42
                                        Layout.preferredHeight: 42
                                        radius: 4
                                        clip: true
                                        color: Qt.rgba(0.73, 0.82, 0.92, 0.46)
                                        Image {
                                            anchors.fill: parent
                                            fillMode: Image.PreserveAspectCrop
                                            property bool isGroupDialog: root.sessionFilter === 2 || (root.sessionFilter === 0 && uid < 0)
                                            property string fallbackSource: isGroupDialog ? "qrc:/res/chat_icon.png" : "qrc:/res/head_1.jpg"
                                            property string baseSource: (icon && icon.length > 0) ? icon : fallbackSource
                                            property bool loadFailed: false
                                            source: loadFailed ? fallbackSource : baseSource
                                            onBaseSourceChanged: loadFailed = false
                                            onStatusChanged: {
                                                if (status === Image.Error) {
                                                    loadFailed = true
                                                }
                                            }
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 3
                                        Label { text: name; font.bold: true; color: "#273449" }
                                        Label {
                                            text: (draftText && draftText.length > 0)
                                                  ? ("草稿: " + draftText)
                                                  : ((lastMsg && lastMsg.length > 0) ? lastMsg : desc)
                                            color: (draftText && draftText.length > 0) ? "#c05f45" : "#647489"
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                        spacing: 4
                                        visible: (unreadCount && unreadCount > 0)
                                                 || (mentionCount && mentionCount > 0)
                                                 || (pinnedRank && pinnedRank > 0)
                                                 || (muteState && muteState > 0)

                                        Rectangle {
                                            Layout.alignment: Qt.AlignRight
                                            width: mentionCount > 99 ? 30 : 22
                                            height: 18
                                            radius: 9
                                            visible: mentionCount && mentionCount > 0
                                            color: "#f09a3e"
                                            Label {
                                                anchors.centerIn: parent
                                                text: mentionCount > 99 ? "@99+" : ("@" + String(mentionCount))
                                                color: "white"
                                                font.pixelSize: 10
                                                font.bold: true
                                            }
                                        }

                                        Rectangle {
                                            Layout.alignment: Qt.AlignRight
                                            width: 18
                                            height: 18
                                            radius: 9
                                            visible: pinnedRank && pinnedRank > 0
                                            color: Qt.rgba(0.32, 0.60, 0.92, 0.24)
                                            border.color: Qt.rgba(0.32, 0.60, 0.92, 0.50)
                                            Label {
                                                anchors.centerIn: parent
                                                text: "置"
                                                color: "#2f5f92"
                                                font.pixelSize: 10
                                            }
                                        }

                                        Rectangle {
                                            Layout.alignment: Qt.AlignRight
                                            width: unreadCount > 99 ? 26 : 20
                                            height: 20
                                            radius: 10
                                            visible: unreadCount && unreadCount > 0
                                            color: "#d95f5f"
                                            Label {
                                                anchors.centerIn: parent
                                                text: unreadCount > 99 ? "99+" : String(unreadCount)
                                                color: "white"
                                                font.pixelSize: 11
                                                font.bold: true
                                            }
                                        }

                                        Rectangle {
                                            Layout.alignment: Qt.AlignRight
                                            width: 18
                                            height: 18
                                            radius: 9
                                            visible: muteState && muteState > 0
                                            color: Qt.rgba(0.42, 0.56, 0.74, 0.24)
                                            border.color: Qt.rgba(0.42, 0.56, 0.74, 0.50)
                                            Label {
                                                anchors.centerIn: parent
                                                text: "静"
                                                color: "#436789"
                                                font.pixelSize: 10
                                            }
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    onClicked: function(mouse) {
                                        if (mouse.button === Qt.RightButton) {
                                            root.activateSession(uid, index)
                                            sessionMenu.popup()
                                            return
                                        }
                                        root.activateSession(uid, index)
                                    }
                                }

                                Menu {
                                    id: sessionMenu
                                    y: parent.height - 4

                                    MenuItem {
                                        text: (pinnedRank && pinnedRank > 0) ? "取消置顶" : "置顶会话"
                                        onTriggered: function() { root.dialogPinToggled(uid) }
                                    }
                                    MenuItem {
                                        text: (muteState && muteState > 0) ? "取消静音" : "静音会话"
                                        onTriggered: function() { root.dialogMuteToggled(uid) }
                                    }
                                    MenuItem {
                                        text: "标记已读"
                                        enabled: unreadCount && unreadCount > 0
                                        onTriggered: function() { root.dialogMarkRead(uid) }
                                    }
                                    MenuItem {
                                        text: "清空草稿"
                                        enabled: draftText && draftText.length > 0
                                        onTriggered: function() { root.dialogDraftCleared(uid) }
                                    }
                                }

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    height: 1
                                    color: Qt.rgba(1, 1, 1, 0.28)
                                    visible: index < (sessionList.count - 1)
                                }
                            }
                        }
                    }

                    GlassSurface {
                        anchors.centerIn: parent
                        width: 180
                        height: 64
                        visible: sessionList.count === 0 && !root.chatLoadingMore
                        backdrop: root.backdrop !== null ? root.backdrop : root
                        cornerRadius: 10
                        blurRadius: 16
                        fillColor: Qt.rgba(1, 1, 1, 0.20)
                        strokeColor: Qt.rgba(1, 1, 1, 0.42)

                        Label {
                            anchors.centerIn: parent
                            text: root.sessionFilter === 2 ? "暂无群聊"
                                  : (root.sessionFilter === 1 ? "暂无私聊" : "暂无会话")
                            color: "#6a7b92"
                            font.pixelSize: 13
                        }
                    }
                }

                ContactListPane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: searchField.text.length === 0 && root.currentTab === 1
                    backdrop: root.backdrop
                    contactModel: root.contactModel
                    hasPendingApply: root.hasPendingApply
                    canLoadMore: root.canLoadMoreContacts
                    loadingMore: root.contactLoadingMore
                    onOpenApplyRequested: root.openApplyRequested()
                    onContactIndexSelected: function(index) { root.contactIndexSelected(index) }
                    onLoadMoreRequested: root.requestContactLoadMore()
                }
            }
        }
    }

    AddFriendDialog {
        id: addFriendDialog
        anchors.centerIn: Overlay.overlay
        backdrop: root.backdrop !== null ? root.backdrop : root
        onSubmitted: function(uid, backName, tags) { root.addFriendRequested(uid, backName, tags) }
    }
}
